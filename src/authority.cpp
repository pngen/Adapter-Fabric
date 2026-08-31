// Adapter Fabric — authority fencing implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/authority.hpp"

namespace adapter_fabric {

const char* to_string(AuthorityVerdict v) noexcept {
  switch (v) {
    case AuthorityVerdict::authorized: return "authorized";
    case AuthorityVerdict::stale_worker: return "stale_worker";
    case AuthorityVerdict::stale_boot: return "stale_boot";
    case AuthorityVerdict::stale_attempt: return "stale_attempt";
    case AuthorityVerdict::stale_adapter_generation: return "stale_adapter_generation";
    case AuthorityVerdict::stale_composition_generation: return "stale_composition_generation";
    case AuthorityVerdict::stale_residency_generation: return "stale_residency_generation";
    case AuthorityVerdict::stale_artifact_generation: return "stale_artifact_generation";
    case AuthorityVerdict::stale_base_model: return "stale_base_model";
    case AuthorityVerdict::stale_epoch: return "stale_epoch";
    case AuthorityVerdict::unknown: return "unknown";
  }
  return "unknown";
}

AuthorityCheck validate_authority(const AuthorityFence& c, const AuthorityFence& ref) {
  AuthorityCheck out;
  auto reject = [&](AuthorityVerdict v, const char* what) {
    out.verdict = v;
    out.reason = std::string("stale ") + what;
  };

  if (c.worker != ref.worker) { reject(AuthorityVerdict::stale_worker, "worker"); return out; }
  if (c.boot != ref.boot) { reject(AuthorityVerdict::stale_boot, "worker boot"); return out; }
  if (c.attempt != ref.attempt) { reject(AuthorityVerdict::stale_attempt, "attempt"); return out; }
  if (c.adapter_generation != ref.adapter_generation) { reject(AuthorityVerdict::stale_adapter_generation, "adapter generation"); return out; }
  if (c.composition_generation != ref.composition_generation) { reject(AuthorityVerdict::stale_composition_generation, "composition generation"); return out; }
  if (c.residency_generation != ref.residency_generation) { reject(AuthorityVerdict::stale_residency_generation, "residency generation"); return out; }
  if (c.artifact_generation != ref.artifact_generation) { reject(AuthorityVerdict::stale_artifact_generation, "artifact generation"); return out; }
  if (c.base_model_revision != ref.base_model_revision) { reject(AuthorityVerdict::stale_base_model, "base model revision"); return out; }
  if (c.epoch != ref.epoch) { reject(AuthorityVerdict::stale_epoch, "coordinator epoch"); return out; }
  out.verdict = AuthorityVerdict::authorized;
  out.reason = "all authority factors current";
  return out;
}

}  // namespace adapter_fabric
