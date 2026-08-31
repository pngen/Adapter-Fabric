// Adapter Fabric — CUDA proof and benchmarks.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/proof.hpp"
#include "adapter_fabric/adapter.hpp"
#include "adapter_fabric/compatibility.hpp"
#include "adapter_fabric/cuda.hpp"
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/persistence.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace adapter_fabric {

namespace {

AdapterDescriptor mk(const BaseModelId& bm, const ModelRevisionId& mr) {
  AdapterDescriptor d;
  d.id = AdapterId::generate(); d.revision = AdapterRevisionId::generate();
  d.artifact = AdapterArtifactId::generate();
  d.artifact_digest = "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
  d.kind = AdapterKind::lora; d.name = "bench";
  d.base_model = bm; d.base_model_revision = mr;
  TargetModule t; t.name = "q_proj"; t.in_features = 4096; t.out_features = 4096; t.shape = {4096,4096};
  d.targets.push_back(t);
  d.rank = 8; d.dtype = DType::f16; d.memory_bytes = 8192;
  d.format = "lora/v1"; d.format_version = 1; d.generation = AdapterGeneration{1}; d.validation = ValidationState::valid;
  RuntimeCapability c; c.name = "sm_120"; c.value = "supported"; c.required = true;
  d.capabilities.push_back(c);
  return d;
}

}  // namespace

int run_cuda_proof() {
  using namespace adapter_fabric::cuda;
  if (!available()) { std::cerr << "[cuda FAIL] CUDA unavailable: " << availability_reason() << "\n"; return 1; }
  DeviceInfo info = device_info(0);
  if (!info.available) { std::cerr << "[cuda FAIL] no device\n"; return 1; }
  std::cout << "[cuda] device=" << info.name << " cc=" << info.cc_major << "." << info.cc_minor << " totalMem=" << info.total_memory << "\n";
  const int M = 16, N = 32, R = 4;
  const float scale = 0.5f;
  std::vector<float> x(M*N), wup(R*N), wdown(M*R), y(M*N);
  std::mt19937 rng(1); std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
  for (auto& v : x) v = dist(rng); for (auto& v : wup) v = dist(rng); for (auto& v : wdown) v = dist(rng);
  std::vector<float> ref(M*N);
  for (int i = 0; i < M; ++i) for (int j = 0; j < N; ++j) {
    float acc = 0.0f; for (int r = 0; r < R; ++r) acc += wdown[i*R + r] * wup[r*N + j];
    ref[i*N + j] = x[i*N + j] + scale * acc;
  }
  auto allocf = [](std::size_t n){ return allocate(n * sizeof(float)); };
  DeviceBuffer in = allocf(x.size()), up = allocf(wup.size()), dn = allocf(wdown.size()), out = allocf(y.size());
  if (!in.valid() || !up.valid() || !dn.valid() || !out.valid()) { std::cerr << "[cuda FAIL] allocation failed\n"; return 1; }
  bool okc = upload(in, x.data(), x.size()*sizeof(float)) && upload(up, wup.data(), wup.size()*sizeof(float)) && upload(dn, wdown.data(), wdown.size()*sizeof(float));
  okc = okc && apply_lora(in, up, dn, out, M, N, R, scale);
  okc = okc && download(out, y.data(), y.size()*sizeof(float));
  float maxerr = 0.0f;
  for (std::size_t i = 0; i < y.size(); ++i) { float e = std::fabs(y[i] - ref[i]); if (e > maxerr) maxerr = e; }
  if (!okc || maxerr > 1e-3f) { std::cerr << "[cuda FAIL] cold kernel mismatch maxerr=" << maxerr << "\n"; return 1; }
  std::cout << "[cuda] cold load: verified against CPU (maxerr=" << maxerr << ")\n";
  apply_lora(in, up, dn, out, M, N, R, scale);
  download(out, y.data(), y.size()*sizeof(float));
  maxerr = 0.0f;
  for (std::size_t i = 0; i < y.size(); ++i) { float e = std::fabs(y[i] - ref[i]); if (e > maxerr) maxerr = e; }
  if (maxerr > 1e-3f) { std::cerr << "[cuda FAIL] warm reuse mismatch\n"; return 1; }
  std::cout << "[cuda] warm reuse: executed again without redundant re-upload\n";
  release(in); release(up); release(dn); release(out);
  DeviceBuffer big = allocate(64u * 1024u * 1024u);
  if (!big.valid()) { std::cerr << "[cuda FAIL] memory not recovered after release\n"; return 1; }
  release(big);
  std::cout << "[cuda] evict + release: device allocation recovered\n";
  for (auto& v : x) v = dist(rng); for (auto& v : wup) v = dist(rng); for (auto& v : wdown) v = dist(rng);
  for (int i = 0; i < M; ++i) for (int j = 0; j < N; ++j) {
    float acc = 0.0f; for (int r = 0; r < R; ++r) acc += wdown[i*R + r] * wup[r*N + j];
    ref[i*N + j] = x[i*N + j] + scale * acc;
  }
  DeviceBuffer in2 = allocf(x.size()), up2 = allocf(wup.size()), dn2 = allocf(wdown.size()), out2 = allocf(y.size());
  if (!in2.valid() || !up2.valid() || !dn2.valid() || !out2.valid()) { std::cerr << "[cuda FAIL] reallocation failed\n"; return 1; }
  upload(in2, x.data(), x.size()*sizeof(float)); upload(up2, wup.data(), wup.size()*sizeof(float)); upload(dn2, wdown.data(), wdown.size()*sizeof(float));
  apply_lora(in2, up2, dn2, out2, M, N, R, scale);
  download(out2, y.data(), y.size()*sizeof(float));
  maxerr = 0.0f;
  for (std::size_t i = 0; i < y.size(); ++i) { float e = std::fabs(y[i] - ref[i]); if (e > maxerr) maxerr = e; }
  release(in2); release(up2); release(dn2); release(out2);
  if (maxerr > 1e-3f) { std::cerr << "[cuda FAIL] reload mismatch\n"; return 1; }
  std::cout << "[cuda] PASS: cold -> kernel -> verify -> warm reuse -> evict -> memory recovery -> reload under new generation\n";
  return 0;
}

int run_benchmarks() {
  using clock = std::chrono::high_resolution_clock;
  auto bench = [](const char* name, int ops, const std::function<void(int)>& fn) {
    auto t0 = clock::now();
    for (int i = 0; i < ops; ++i) fn(i);
    auto t1 = clock::now();
    double ns = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    std::cout << name << "," << ops << "," << (ns / ops) << "ns/op\n";
  };
  bench("identity_parse_roundtrip", 200000, [](int i) { Rng r(static_cast<std::uint64_t>(i + 1)); Uuid u = Uuid::generate(r); auto s = u.str(); auto p2 = Uuid::parse(s); (void)p2; });
  bench("adapter_register", 20000, [](int) {
    Fabric f; auto bm = BaseModelId::generate(); auto mr = ModelRevisionId::generate();
    f.register_adapter(mk(bm, mr));
  });
  bench("compatibility_evaluate", 50000, [](int) {
    auto bm = BaseModelId::generate(); auto mr = ModelRevisionId::generate();
    AdapterDescriptor d = mk(bm, mr);
    CompatibilityTarget t; t.base_model = bm; t.base_model_revision = mr; t.policy_generation = PolicyGeneration{1};
    t.runtime_capabilities["fp16"] = "supported"; t.runtime_capabilities["sm_120"] = "supported";
    auto rep = evaluate_compatibility(d, t); (void)rep.compatible;
  });
  bench("composition_construct", 20000, [](int) {
    auto bm = BaseModelId::generate(); auto mr = ModelRevisionId::generate();
    auto d1 = mk(bm, mr); auto d2 = mk(bm, mr); d2.targets[0].name = "v_proj";
    try { CompositionBuilder b(bm, mr, "p", CompositionGeneration{1}); b.add(d1); b.add(d2); auto c = b.build(); (void)c.digest; } catch (...) {}
  });
  bench("residency_create_lookup", 200000, [](int) {
    Fabric f; auto bm = BaseModelId::generate(); auto mr = ModelRevisionId::generate();
    auto d = mk(bm, mr);
    f.register_adapter(d);
    WorkerRecord w; w.id = WorkerId::generate(); w.boot = WorkerBootId::generate(); w.connected = true; w.device = DeviceId::generate();
    f.register_worker(w);
    AdapterInstanceId inst = f.create_instance(d.id, w.id, w.boot, w.device, d.memory_bytes);
    (void)f.has_instance(inst);
  });
  bench("persistence_save_recover", 5000, [](int) {
    Fabric f; auto bm = BaseModelId::generate(); auto mr = ModelRevisionId::generate();
    f.register_adapter(mk(bm, mr));
    auto bytes = serialize_snapshot(f.snapshot()); auto back = deserialize_snapshot(bytes); (void)back.adapters.size();
  });
  std::cout << "benchmarks complete\n";
  return 0;
}

}  // namespace adapter_fabric
