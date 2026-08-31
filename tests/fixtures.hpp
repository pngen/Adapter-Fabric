// Adapter Fabric — test fixtures.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "adapter_fabric/adapter.hpp"
#include "adapter_fabric/compatibility.hpp"
#include "adapter_fabric/fabric.hpp"

namespace af_test_fixture {

inline adapter_fabric::BaseModelId base() { return adapter_fabric::BaseModelId::generate(); }
inline adapter_fabric::ModelRevisionId rev() { return adapter_fabric::ModelRevisionId::generate(); }

inline adapter_fabric::AdapterDescriptor make_lora(const std::string& name,
    const adapter_fabric::BaseModelId& bm, const adapter_fabric::ModelRevisionId& mr,
    std::uint64_t mem = 4096, std::uint32_t rank = 8, std::uint32_t format_version = 1) {
  adapter_fabric::AdapterDescriptor d;
  d.id = adapter_fabric::AdapterId::generate();
  d.revision = adapter_fabric::AdapterRevisionId::generate();
  d.artifact = adapter_fabric::AdapterArtifactId::generate();
  d.artifact_digest = "0123456789abcdef0123456789abcdef";
  d.kind = adapter_fabric::AdapterKind::lora;
  d.name = name;
  d.base_model = bm;
  d.base_model_revision = mr;
  adapter_fabric::TargetModule t; t.name = "q_proj"; t.in_features = 4096; t.out_features = 4096; t.shape = {4096, 4096};
  d.targets.push_back(t);
  d.rank = rank;
  d.dtype = adapter_fabric::DType::f16;
  d.param_count = mem / 2;
  d.param_bytes = mem / 2;
  d.memory_bytes = mem;
  d.format = "lora/v1";
  d.format_version = format_version;
  d.generation = adapter_fabric::AdapterGeneration{1};
  d.validation = adapter_fabric::ValidationState::valid;
  adapter_fabric::RuntimeCapability c; c.name = "sm_120"; c.value = "supported"; c.required = true;
  d.capabilities.push_back(c);
  return d;
}

inline adapter_fabric::CompatibilityTarget target(
    const adapter_fabric::BaseModelId& bm, const adapter_fabric::ModelRevisionId& mr) {
  adapter_fabric::CompatibilityTarget t;
  t.base_model = bm;
  t.base_model_revision = mr;
  t.architecture = "llama";
  t.device_arch = "sm_120";
  t.policy_generation = adapter_fabric::PolicyGeneration{1};
  t.runtime_capabilities["fp16"] = "supported";
  t.runtime_capabilities["sm_120"] = "supported";
  return t;
}

}  // namespace af_test_fixture