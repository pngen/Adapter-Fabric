// Adapter Fabric — invalidation tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "fixtures.hpp"
#include "adapter_fabric/fabric.hpp"

using namespace adapter_fabric;
namespace fx = af_test_fixture;

AF_TEST_CASE(invalidation_clears_authority_and_logs) {
  Fabric f;
  auto bm = fx::base(); auto mr = fx::rev();
  AdapterDescriptor d = f.register_adapter(fx::make_lora("a", bm, mr));
  WorkerRecord w; w.id = WorkerId::generate(); w.boot = WorkerBootId::generate(); w.connected = true; w.device = DeviceId::generate();
  f.register_worker(w);
  auto dev = DeviceId::generate();
  f.create_instance(d.id, w.id, w.boot, dev, d.memory_bytes);
  AuthorityFence fence; fence.adapter = d.id; fence.epoch = f.epoch(); fence.adapter_generation = d.generation;
  fence.base_model_revision = d.base_model_revision; fence.worker = w.id; fence.boot = w.boot; fence.attempt = AttemptId::generate();
  fence.device = dev; fence.residency_generation = ResidencyGeneration{1}; fence.composition_generation = CompositionGeneration{1};
  fence.artifact_generation = ArtifactGeneration{1};
  f.bind_authority(d.id, fence);
  auto rec = f.invalidate(d.id, InvalidationTrigger::base_model_revision_change, "model revision changed");
  AF_CHECK(rec.trigger == InvalidationTrigger::base_model_revision_change);
  AF_CHECK(f.invalidation_log().size() == 1);
  AF_CHECK(!f.authority_record(d.id)->active);
  AF_CHECK_EQ(rec.reason, std::string("model revision changed"));
}

AF_TEST_MAIN
