// Adapter Fabric — CLI.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/adapter.hpp"
#include "adapter_fabric/compatibility.hpp"
#include "adapter_fabric/composition.hpp"
#include "adapter_fabric/explain.hpp"
#include "adapter_fabric/coordinator.hpp"
#include "adapter_fabric/worker.hpp"
#include "adapter_fabric/persistence.hpp"
#include "adapter_fabric/proof.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <variant>
#include <thread>
#include <chrono>

using namespace adapter_fabric;

namespace {
template <typename IdT>
IdT must_parse(std::string_view s) { auto v = IdT::parse(s); if (auto* id = std::get_if<IdT>(&v)) return *id; throw Error(ErrorCode::invalid_identity, "invalid identity"); }

AdapterDescriptor make_desc(const std::string& name, const BaseModelId& bm, const ModelRevisionId& mr, std::uint64_t mem) {
  AdapterDescriptor d;
  d.id = AdapterId::generate(); d.revision = AdapterRevisionId::generate();
  d.artifact = AdapterArtifactId::generate();
  d.artifact_digest = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  d.kind = AdapterKind::lora; d.name = name;
  d.base_model = bm; d.base_model_revision = mr;
  TargetModule t; t.name = "q_proj"; t.in_features = 4096; t.out_features = 4096; t.shape = {4096,4096};
  d.targets.push_back(t);
  d.rank = 8; d.dtype = DType::f16; d.memory_bytes = mem ? mem : 8192;
  d.format = "lora/v1"; d.format_version = 1; d.validation = ValidationState::valid;
  return d;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) { std::cerr << "usage: af_cli <command> [args]\n"; return 2; }
  std::string cmd = argv[1];

  if (cmd == "serve") {
    std::uint16_t port = 24000;
    for (int i = 2; i < argc; ++i) if (std::strcmp(argv[i], "--port") == 0 && i+1 < argc) port = static_cast<std::uint16_t>(std::atoi(argv[++i]));
    CoordinatorServer s(port); if (!s.start()) { std::cerr << "failed to start coordinator\n"; return 1; }
    std::cout << "coordinator listening on " << port << "\n";
    while (s.running()) std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return 0;
  }
  if (cmd == "worker") {
    WorkerOptions o; o.coordinator_host = "127.0.0.1"; o.coordinator_port = 24000; o.name = "worker";
    for (int i = 2; i < argc; ++i) {
      if (std::strcmp(argv[i], "--port") == 0 && i+1 < argc) o.coordinator_port = static_cast<std::uint16_t>(std::atoi(argv[++i]));
      else if (std::strcmp(argv[i], "--host") == 0 && i+1 < argc) o.coordinator_host = argv[++i];
      else if (std::strcmp(argv[i], "--name") == 0 && i+1 < argc) o.name = argv[++i];
      else if (std::strcmp(argv[i], "--seed") == 0 && i+1 < argc) o.boot_seed = std::strtoull(argv[++i], nullptr, 10);
    }
    o.id = WorkerId::generate(); o.node = NodeId::generate(); o.device = DeviceId::generate();
    WorkerService w(std::move(o)); if (!w.connect_and_register()) { std::cerr << "worker register failed\n"; return 1; }
    std::cout << "worker boot=" << w.boot().str() << "\n"; w.run(); return 0;
  }
  if (cmd == "multiprocess") {
    if (argc < 4) { std::cerr << "usage: af_cli multiprocess <coord_exe> <worker_exe>\n"; return 2; }
    return run_multiprocess_proof(argv[2], argv[3], 23002);
  }
  if (cmd == "cuda") { return run_cuda_proof(); }
  if (cmd == "benchmark") { return run_benchmarks(); }

  Fabric f;
  WorkerId worker = WorkerId::generate(); WorkerBootId boot = WorkerBootId::generate(); DeviceId dev = DeviceId::generate();
  WorkerRecord wrec; wrec.id = worker; wrec.boot = boot; wrec.connected = true; wrec.device = dev;
  f.register_worker(wrec);

  try {
    if (cmd == "register") { std::string name = argc > 2 ? argv[2] : "adapter"; auto d = f.register_adapter(make_desc(name, BaseModelId::generate(), ModelRevisionId::generate(), 8192)); std::cout << "registered " << d.name << " id=" << d.id.str() << " gen=" << d.generation.value() << "\n"; return 0; }
    if (cmd == "list") { for (auto& a : f.adapters()) std::cout << a.id.str() << "  gen=" << a.generation.value() << "  " << a.name << "  " << a.memory_bytes << "B\n"; return 0; }
    if (cmd == "inspect") { auto id = must_parse<AdapterId>(argv[2]); auto* d = f.adapter(id); if (!d) { std::cerr << "unknown adapter\n"; return 1; } std::cout << "name=" << d->name << " kind=" << to_string(d->kind) << " rank=" << d->rank << " mem=" << d->memory_bytes << " gen=" << d->generation.value() << "\n"; return 0; }
    if (cmd == "validate") { auto id = must_parse<AdapterId>(argv[2]); auto* d = f.adapter(id); if (!d) { std::cerr << "unknown adapter\n"; return 1; } CompatibilityTarget target; target.base_model = d->base_model; target.base_model_revision = d->base_model_revision; target.policy_generation = PolicyGeneration{1}; target.runtime_capabilities["fp16"] = "supported"; target.runtime_capabilities["sm_120"] = "supported"; auto rep = f.validate(id, target); std::cout << (rep.compatible ? "compatible" : "incompatible") << ": " << rep.summary << "\n" << to_json(rep) << "\n"; return rep.compatible ? 0 : 1; }
    if (cmd == "compose") { std::vector<AdapterId> ids; for (int i = 2; i < argc; ++i) ids.push_back(must_parse<AdapterId>(argv[i])); auto c = f.publish_composition(ids, "default"); std::cout << "composition id=" << c.id.str() << " gen=" << c.generation.value() << " digest=" << c.digest << " mem=" << c.total_memory_bytes << "\n"; return 0; }
    if (cmd == "explain") { auto id = must_parse<AdapterId>(argv[2]); auto* d = f.adapter(id); if (!d) { std::cerr << "unknown adapter\n"; return 1; } CompatibilityTarget target; target.base_model = d->base_model; target.base_model_revision = d->base_model_revision; target.policy_generation = PolicyGeneration{1}; target.runtime_capabilities["fp16"] = "supported"; auto rep = f.validate(id, target); auto ex = explain_compatibility(rep); std::cout << ex.text << "\n" << ex.json << "\n"; return 0; }
    if (cmd == "load") { auto id = must_parse<AdapterId>(argv[2]); std::uint64_t mem = f.adapter(id) ? f.adapter(id)->memory_bytes : 0; auto inst = f.create_instance(id, worker, boot, dev, mem); auto* rec = f.residency(inst); if (rec) rec->ready = true; std::cout << "loaded instance=" << inst.str() << "\n"; return 0; }
    if (cmd == "activate") { auto id = must_parse<AdapterId>(argv[2]); auto* d = f.adapter(id); if (!d) { std::cerr << "unknown adapter\n"; return 1; } AuthorityFence fe; fe.adapter = id; fe.adapter_generation = d->generation; fe.base_model_revision = d->base_model_revision; fe.epoch = f.epoch(); fe.worker = worker; fe.boot = boot; fe.device = dev; fe.attempt = AttemptId::generate(); fe.residency_generation = ResidencyGeneration{1}; fe.composition_generation = CompositionGeneration{1}; fe.artifact_generation = ArtifactGeneration{1}; auto chk = f.bind_authority(id, fe); std::cout << (chk.authorized() ? "activated" : ("DENIED: " + chk.reason)) << "\n"; return chk.authorized() ? 0 : 1; }
    if (cmd == "deactivate") { auto id = must_parse<AdapterId>(argv[2]); auto* ar = const_cast<AdapterAuthorityRecord*>(f.authority_record(id)); if (ar) { ar->active = false; ar->lifecycle.set_state(LifecycleState::ready); } std::cout << "deactivated\n"; return 0; }
    if (cmd == "invalidate") { auto id = must_parse<AdapterId>(argv[2]); std::string reason = argc > 3 ? argv[3] : "manual"; auto rec = f.invalidate(id, InvalidationTrigger::manual, reason); std::cout << "invalidated: " << rec.reason << "\n"; return 0; }
    if (cmd == "evict") { auto inst = must_parse<AdapterInstanceId>(argv[2]); std::string reason; bool ok = f.evict(inst, reason); std::cout << (ok ? "evicted: " : "DENIED: ") << reason << "\n"; return ok ? 0 : 1; }
    if (cmd == "pin") { f.pin(must_parse<AdapterInstanceId>(argv[2])); std::cout << "pinned\n"; return 0; }
    if (cmd == "unpin") { f.unpin(must_parse<AdapterInstanceId>(argv[2])); std::cout << "unpinned\n"; return 0; }
    if (cmd == "snapshot") { auto bytes = serialize_snapshot(f.snapshot()); std::cout << "snapshot " << bytes.size() << " bytes\n"; return 0; }
    if (cmd == "save") { std::string file = argc > 2 ? argv[2] : "snap.bin"; auto bytes = serialize_snapshot(f.snapshot()); std::ofstream os(file, std::ios::binary); os.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())); std::cout << "saved " << bytes.size() << " bytes to " << file << "\n"; return 0; }
    if (cmd == "recover") { std::string file = argc > 2 ? argv[2] : "snap.bin"; std::ifstream is(file, std::ios::binary); std::vector<char> b((std::istreambuf_iterator<char>(is)), {}); std::vector<std::uint8_t> bytes(b.begin(), b.end()); f.recover(deserialize_snapshot(bytes)); std::cout << "recovered from " << file << " (" << bytes.size() << " bytes)\n"; return 0; }
    std::cerr << "unknown command: " << cmd << "\n"; return 2;
  } catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 1; }
}
