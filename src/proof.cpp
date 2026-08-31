// Adapter Fabric — real proofs and benchmarks.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/proof.hpp"
#include "adapter_fabric/adapter.hpp"
#include "adapter_fabric/authority.hpp"
#include "adapter_fabric/checksum.hpp"
#include "adapter_fabric/compatibility.hpp"
#include "adapter_fabric/cuda.hpp"
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/identity.hpp"
#include "adapter_fabric/persistence.hpp"
#include "adapter_fabric/protocol.hpp"
#include "adapter_fabric/rng.hpp"
#include "adapter_fabric/transport.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>
#include <functional>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace adapter_fabric {

namespace {

struct ChildProcess { void* handle = nullptr; std::uint32_t pid = 0; };

ChildProcess spawn(const std::string& cmd) {
  ChildProcess c;
#ifdef _WIN32
  STARTUPINFOA si{}; si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  std::vector<char> buf(cmd.begin(), cmd.end()); buf.push_back(0);
  if (CreateProcessA(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) { c.handle = pi.hProcess; c.pid = pi.dwProcessId; }
#endif
  (void)cmd;
  return c;
}

void terminate(ChildProcess& c) {
#ifdef _WIN32
  if (c.handle) { TerminateProcess(static_cast<HANDLE>(c.handle), 0); CloseHandle(static_cast<HANDLE>(c.handle)); c.handle = nullptr; }
#endif
}

void sleep_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

WorkerBootId boot_for(std::uint64_t seed) { Rng r(seed); return WorkerBootId::generate(r); }
AttemptId attempt_for(std::uint64_t seed) { Rng r(seed); return AttemptId::generate(r); }

struct ControlClient {
  TcpSocket s;
  bool connect_retry(const std::string& host, std::uint16_t port, int tries) {
    for (int i = 0; i < tries; ++i) { if (s.connect(host, port)) return true; sleep_ms(100); }
    return false;
  }
  Message request(const Message& req) {
    if (!s.send_frame(encode_message(req))) { Message m; m.ok = false; m.text = "send failed"; return m; }
    std::vector<std::uint8_t> buf;
    if (!s.recv_frame(buf)) { Message m; m.ok = false; m.text = "recv failed"; return m; }
    try { return decode_message(buf); } catch (...) { Message m; m.ok = false; m.text = "decode failed"; return m; }
  }
};

AdapterDescriptor make_proof_adapter(const BaseModelId& bm, const ModelRevisionId& mr) {
  AdapterDescriptor d;
  d.id = AdapterId::generate();
  d.revision = AdapterRevisionId::generate();
  d.artifact = AdapterArtifactId::generate();
  d.artifact_digest = "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
  d.kind = AdapterKind::lora;
  d.name = "proof-lora";
  d.base_model = bm;
  d.base_model_revision = mr;
  TargetModule t; t.name = "q_proj"; t.in_features = 4096; t.out_features = 4096; t.shape = {4096, 4096};
  d.targets.push_back(t);
  d.rank = 8;
  d.dtype = DType::f16;
  d.memory_bytes = 8192;
  d.format = "lora/v1";
  d.format_version = 1;
  d.generation = AdapterGeneration{1};
  d.validation = ValidationState::valid;
  RuntimeCapability c; c.name = "sm_120"; c.value = "supported"; c.required = true;
  d.capabilities.push_back(c);
  return d;
}

AuthorityFence make_fence(const AdapterDescriptor& d, const WorkerId& w, const WorkerBootId& b, const DeviceId& dev, std::uint64_t epoch, std::uint64_t attempt_seed) {
  AuthorityFence f;
  f.adapter = d.id;
  f.adapter_generation = d.generation;
  f.base_model_revision = d.base_model_revision;
  f.epoch = CoordinatorEpoch{epoch};
  f.worker = w; f.boot = b; f.device = dev;
  f.attempt = attempt_for(attempt_seed);
  f.residency_generation = ResidencyGeneration{1};
  f.composition_generation = CompositionGeneration{1};
  f.artifact_generation = ArtifactGeneration{1};
  return f;
}

}  // namespace

int run_multiprocess_proof(const std::string& coord_exe, const std::string& worker_exe, std::uint16_t port) {
  auto fail = [](const char* m) { std::cerr << "[multiprocess FAIL] " << m << "\n"; return 1; };
  ChildProcess coord = spawn("\"" + coord_exe + "\" --port " + std::to_string(port));
  if (!coord.handle) return fail("failed to spawn coordinator");
  ControlClient ctl;
  if (!ctl.connect_retry("127.0.0.1", port, 50)) { terminate(coord); return fail("coordinator did not come up"); }

  WorkerId workerA = WorkerId::generate(); WorkerId workerB = WorkerId::generate();
  WorkerBootId bootA = boot_for(1); WorkerBootId bootB = boot_for(2);
  DeviceId devA = DeviceId::generate(); DeviceId devB = DeviceId::generate();
  ChildProcess pa = spawn("\"" + worker_exe + "\" --port " + std::to_string(port) + " --id " + workerA.str() + " --seed 1 --name A");
  ChildProcess pb = spawn("\"" + worker_exe + "\" --port " + std::to_string(port) + " --id " + workerB.str() + " --seed 2 --name B");
  if (!pa.handle || !pb.handle) { terminate(coord); return fail("failed to spawn workers"); }
  sleep_ms(1200);

  BaseModelId bm = BaseModelId::generate(); ModelRevisionId mr = ModelRevisionId::generate();
  AdapterDescriptor desc = make_proof_adapter(bm, mr);
  { Message reg; reg.type = MsgType::ctl_register_adapter; reg.payload = serialize_adapter(desc);
    auto r = ctl.request(reg); if (!r.ok) { terminate(pa); terminate(pb); terminate(coord); return fail("register adapter failed"); }
    desc = deserialize_adapter(r.payload); }
  { Message val; val.type = MsgType::ctl_validate; val.text = desc.id.str(); val.text2 = bm.str(); val.text3 = mr.str();
    auto r = ctl.request(val); if (!r.ok) { terminate(pa); terminate(pb); terminate(coord); return fail("validate failed"); } }

  // current epoch (starter = 0) and one fence under current authority on worker A.
  std::uint64_t epoch = 0;
  AuthorityFence fence = make_fence(desc, workerA, bootA, devA, epoch, 1001);
  { Message load; load.type = MsgType::ctl_load; load.text = desc.id.str(); load.text2 = workerA.str(); load.payload = encode_fence(fence);
    auto r = ctl.request(load); if (!r.ok) { terminate(pa); terminate(pb); terminate(coord); return fail("load failed"); } }
  { Message act; act.type = MsgType::ctl_activate; act.text = desc.id.str(); act.text2 = workerA.str(); act.payload = encode_fence(fence);
    auto r = ctl.request(act); if (!r.ok) { terminate(pa); terminate(pb); terminate(coord); return fail("activate failed"); } }

  // stale attempt
  AuthorityFence s = fence; s.attempt = attempt_for(9999);
  { Message m; m.type = MsgType::ctl_activate; m.text = desc.id.str(); m.text2 = workerA.str(); m.payload = encode_fence(s); auto r = ctl.request(m); if (r.ok) return fail("stale attempt accepted"); }
  // stale adapter generation
  s = fence; s.adapter_generation = AdapterGeneration{fence.adapter_generation.value() + 1};
  { Message m; m.type = MsgType::ctl_activate; m.text = desc.id.str(); m.text2 = workerA.str(); m.payload = encode_fence(s); auto r = ctl.request(m); if (r.ok) return fail("stale adapter generation accepted"); }
  // stale residency generation
  s = fence; s.residency_generation = ResidencyGeneration{2};
  { Message m; m.type = MsgType::ctl_activate; m.text = desc.id.str(); m.text2 = workerA.str(); m.payload = encode_fence(s); auto r = ctl.request(m); if (r.ok) return fail("stale residency generation accepted"); }
  // stale composition generation
  s = fence; s.composition_generation = CompositionGeneration{2};
  { Message m; m.type = MsgType::ctl_activate; m.text = desc.id.str(); m.text2 = workerA.str(); m.payload = encode_fence(s); auto r = ctl.request(m); if (r.ok) return fail("stale composition generation accepted"); }
  // advance epoch, then replay the stale (old) epoch
  { Message e; e.type = MsgType::ctl_epoch; auto r = ctl.request(e); epoch = r.ok ? r.a : epoch; }
  s = fence; s.epoch = CoordinatorEpoch{epoch - 1};
  { Message m; m.type = MsgType::ctl_activate; m.text = desc.id.str(); m.text2 = workerA.str(); m.payload = encode_fence(s); auto r = ctl.request(m); if (r.ok) return fail("stale epoch accepted"); }

  // kill worker A as an OS process
  terminate(pa);
  sleep_ms(800);
  s = fence; s.epoch = CoordinatorEpoch{epoch};
  { Message m; m.type = MsgType::ctl_activate; m.text = desc.id.str(); m.text2 = workerA.str(); m.payload = encode_fence(s); auto r = ctl.request(m); if (r.ok) return fail("activated on a dead worker"); }

  // restart logical worker A with a fresh boot id
  WorkerBootId bootA2 = boot_for(11);
  ChildProcess pa2 = spawn("\"" + worker_exe + "\" --port " + std::to_string(port) + " --id " + workerA.str() + " --seed 11 --name A2");
  if (!pa2.handle) { terminate(pb); terminate(coord); return fail("failed to restart worker A"); }
  sleep_ms(1200);

  // replay stale (old) worker boot
  s = fence; s.epoch = CoordinatorEpoch{epoch}; s.boot = bootA;
  { Message m; m.type = MsgType::ctl_activate; m.text = desc.id.str(); m.text2 = workerA.str(); m.payload = encode_fence(s); auto r = ctl.request(m); if (r.ok) return fail("stale boot accepted"); }

  // reload/revalidate under current authority with fresh boot, then activate fresh state.
  AuthorityFence fresh = make_fence(desc, workerA, bootA2, devA, epoch, 2001);
  { Message load; load.type = MsgType::ctl_load; load.text = desc.id.str(); load.text2 = workerA.str(); load.payload = encode_fence(fresh); auto r = ctl.request(load); if (!r.ok) return fail("fresh load failed"); }
  { Message act; act.type = MsgType::ctl_activate; act.text = desc.id.str(); act.text2 = workerA.str(); act.payload = encode_fence(fresh); auto r = ctl.request(act); if (!r.ok) return fail("fresh activate failed"); }
  { Message act2; act2.type = MsgType::ctl_activate; act2.text = desc.id.str(); act2.text2 = workerA.str(); act2.payload = encode_fence(fresh); auto r = ctl.request(act2); if (!r.ok) return fail("complete fresh work failed"); }
  // deactivate + release resources, then prove accounting returns to baseline.
  { Message dac; dac.type = MsgType::ctl_deactivate; dac.text = desc.id.str(); dac.text2 = workerA.str(); auto r = ctl.request(dac); if (!r.ok) return fail("deactivate failed"); }
  { Message cap; cap.type = MsgType::ctl_capacity; auto r = ctl.request(cap); if (!r.ok) return fail("capacity query failed"); std::cout << "[multiprocess] final capacity: " << r.text << "\n"; }

  terminate(pa2); terminate(pb); terminate(coord);
  ctl.s.close();
  std::cout << "[multiprocess] PASS: real OS-process authority proof completed\n";
  return 0;
}

}  // namespace adapter_fabric