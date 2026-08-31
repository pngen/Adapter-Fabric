// Adapter Fabric — activation authority tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "fixtures.hpp"
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/authority.hpp"

using namespace adapter_fabric;
namespace fx = af_test_fixture;

static AuthorityFence make_fence(Fabric& f, const AdapterDescriptor& d, const WorkerRecord& w, AttemptId attempt, DeviceId dev, AuthorityFence* base = nullptr) {
  AuthorityFence fence;
  fence.adapter = d.id;
  fence.adapter_generation = d.generation;
  fence.base_model_revision = d.base_model_revision;
  fence.epoch = f.epoch();
  fence.worker = w.id;
  fence.boot = w.boot;
  fence.attempt = attempt;
  fence.device = dev;
  fence.residency_generation = ResidencyGeneration{1};
  fence.composition_generation = CompositionGeneration{1};
  fence.artifact_generation = ArtifactGeneration{1};
  fence.composition = CompositionId::nil();
  fence.node = NodeId::nil();
  if (base) { fence = *base; fence.epoch = f.epoch(); fence.adapter_generation = d.generation; }
  return fence;
}

AF_TEST_CASE(authority_current_fence_authorized) {
  Fabric f;
  auto bm = fx::base();
  auto mr = fx::rev();
  AdapterDescriptor d = f.register_adapter(fx::make_lora("a", bm, mr));
  WorkerRecord w; w.id = WorkerId::generate(); w.boot = WorkerBootId::generate(); w.connected = true; w.device = DeviceId::generate();
  f.register_worker(w);
  auto dev = DeviceId::generate();
  f.create_instance(d.id, w.id, w.boot, dev, d.memory_bytes);
  AuthorityFence fence = make_fence(f, d, w, AttemptId::generate(), dev);
  auto chk = f.check_fence(fence);
  AF_CHECK(chk.authorized());
  auto bind = f.bind_authority(d.id, fence);
  AF_CHECK(bind.authorized());
  AF_CHECK(f.authority_record(d.id)->active);
}

AF_TEST_CASE(authority_stale_epoch_rejected) {
  Fabric f;
  auto bm = fx::base(); auto mr = fx::rev();
  AdapterDescriptor d = f.register_adapter(fx::make_lora("a", bm, mr));
  WorkerRecord w; w.id = WorkerId::generate(); w.boot = WorkerBootId::generate(); w.connected = true; w.device = DeviceId::generate();
  f.register_worker(w);
  auto dev = DeviceId::generate();
  f.create_instance(d.id, w.id, w.boot, dev, d.memory_bytes);
  AuthorityFence fence = make_fence(f, d, w, AttemptId::generate(), dev);
  f.bind_authority(d.id, fence);
  // advance epoch, then a fence with the old epoch must be rejected.
  f.advance_epoch();
  AuthorityFence stale = fence;   // stale epoch
  auto chk = f.check_fence(stale);
  AF_CHECK(!chk.authorized());
  AF_CHECK(chk.verdict == AuthorityVerdict::stale_epoch || chk.verdict == AuthorityVerdict::stale_boot ||
            chk.verdict == AuthorityVerdict::stale_attempt || chk.verdict == AuthorityVerdict::stale_adapter_generation);
}

AF_TEST_CASE(authority_stale_boot_rejected) {
  Fabric f;
  auto bm = fx::base(); auto mr = fx::rev();
  AdapterDescriptor d = f.register_adapter(fx::make_lora("a", bm, mr));
  WorkerRecord w; w.id = WorkerId::generate(); w.boot = WorkerBootId::generate(); w.connected = true; w.device = DeviceId::generate();
  f.register_worker(w);
  auto dev = DeviceId::generate();
  f.create_instance(d.id, w.id, w.boot, dev, d.memory_bytes);
  AuthorityFence fence = make_fence(f, d, w, AttemptId::generate(), dev);
  f.bind_authority(d.id, fence);
  AuthorityFence stale = fence;
  stale.boot = WorkerBootId::generate();  // stale boot
  auto chk = f.check_fence(stale);
  AF_CHECK(!chk.authorized());
}

AF_TEST_CASE(authority_stale_attempt_rejected) {
  Fabric f;
  auto bm = fx::base(); auto mr = fx::rev();
  AdapterDescriptor d = f.register_adapter(fx::make_lora("a", bm, mr));
  WorkerRecord w; w.id = WorkerId::generate(); w.boot = WorkerBootId::generate(); w.connected = true; w.device = DeviceId::generate();
  f.register_worker(w);
  auto dev = DeviceId::generate();
  f.create_instance(d.id, w.id, w.boot, dev, d.memory_bytes);
  AuthorityFence fence = make_fence(f, d, w, AttemptId::generate(), dev);
  f.bind_authority(d.id, fence);
  AuthorityFence stale = fence;
  stale.attempt = AttemptId::generate();
  auto chk = f.check_fence(stale);
  AF_CHECK(!chk.authorized());
}

AF_TEST_CASE(authority_worker_restart_clears_authority) {
  Fabric f;
  auto bm = fx::base(); auto mr = fx::rev();
  AdapterDescriptor d = f.register_adapter(fx::make_lora("a", bm, mr));
  WorkerRecord w; w.id = WorkerId::generate(); w.boot = WorkerBootId::generate(); w.connected = true; w.device = DeviceId::generate();
  f.register_worker(w);
  auto dev = DeviceId::generate();
  f.create_instance(d.id, w.id, w.boot, dev, d.memory_bytes);
  AuthorityFence fence = make_fence(f, d, w, AttemptId::generate(), dev);
  f.bind_authority(d.id, fence);
  AF_CHECK(f.authority_record(d.id)->active);
  // Worker restarts with a fresh boot id.
  WorkerBootId new_boot = WorkerBootId::generate();
  f.on_worker_restart(w.id, new_boot);
  AF_CHECK(!f.authority_record(d.id)->active);
  // The old fence (old boot) is now rejected even without an active authority.
  WorkerRecord w2 = w; w2.boot = new_boot;
  f.register_worker(w2);
  auto chk = f.check_fence(fence);
  AF_CHECK(!chk.authorized());
  AF_CHECK(chk.verdict == AuthorityVerdict::stale_boot);
}

AF_TEST_CASE(authority_exclusivity_conflict_throws) {
  Fabric f;
  auto bm = fx::base(); auto mr = fx::rev();
  AdapterDescriptor d1 = f.register_adapter(fx::make_lora("a", bm, mr));
  AdapterDescriptor d2 = f.register_adapter(fx::make_lora("b", bm, mr));
  WorkerRecord w; w.id = WorkerId::generate(); w.boot = WorkerBootId::generate(); w.connected = true; w.device = DeviceId::generate();
  f.register_worker(w);
  auto dev = DeviceId::generate();
  f.create_instance(d1.id, w.id, w.boot, dev, d1.memory_bytes);
  f.create_instance(d2.id, w.id, w.boot, dev, d2.memory_bytes);
  AuthorityFence f1 = make_fence(f, d1, w, AttemptId::generate(), dev);
  f.bind_authority(d1.id, f1);
  AuthorityFence f2 = make_fence(f, d2, w, AttemptId::generate(), dev);
  AF_CHECK_THROWS(f.bind_authority(d2.id, f2));
}

AF_TEST_MAIN