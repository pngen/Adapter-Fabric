// Adapter Fabric — explicit invalidation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <string>
#include "adapter_fabric/identity.hpp"
#include "adapter_fabric/generation.hpp"

namespace adapter_fabric {

enum class InvalidationTrigger : std::uint8_t {
  adapter_revision_change = 0,
  artifact_digest_change,
  base_model_revision_change,
  compatibility_policy_change,
  dependency_change,
  worker_restart,
  device_loss,
  residency_generation_roll,
  composition_change,
  authority_epoch_roll,
  corruption,
  manual,
};

const char* to_string(InvalidationTrigger t) noexcept;

// A record explaining exactly why prior state is no longer reusable.
struct InvalidationRecord {
  InvalidationTrigger trigger;
  AdapterId adapter;
  std::string reason;
  CoordinatorEpoch epoch;
  AdapterGeneration prior_generation;
  std::string detail;
};

}  // namespace adapter_fabric
