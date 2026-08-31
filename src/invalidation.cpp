// Adapter Fabric — invalidation helpers.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/invalidation.hpp"

namespace adapter_fabric {

const char* to_string(InvalidationTrigger t) noexcept {
  switch (t) {
    case InvalidationTrigger::adapter_revision_change: return "adapter_revision_change";
    case InvalidationTrigger::artifact_digest_change: return "artifact_digest_change";
    case InvalidationTrigger::base_model_revision_change: return "base_model_revision_change";
    case InvalidationTrigger::compatibility_policy_change: return "compatibility_policy_change";
    case InvalidationTrigger::dependency_change: return "dependency_change";
    case InvalidationTrigger::worker_restart: return "worker_restart";
    case InvalidationTrigger::device_loss: return "device_loss";
    case InvalidationTrigger::residency_generation_roll: return "residency_generation_roll";
    case InvalidationTrigger::composition_change: return "composition_change";
    case InvalidationTrigger::authority_epoch_roll: return "authority_epoch_roll";
    case InvalidationTrigger::corruption: return "corruption";
    case InvalidationTrigger::manual: return "manual";
  }
  return "unknown";
}

}  // namespace adapter_fabric
