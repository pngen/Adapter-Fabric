// Adapter Fabric — activation authority fencing.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <string>
#include "adapter_fabric/identity.hpp"
#include "adapter_fabric/generation.hpp"

namespace adapter_fabric {

// A complete activation fence: everything an execution-relevant activation
// binds to. Any stale component must never promote adapter state into
// execution authority.
struct AuthorityFence {
  AdapterId adapter;
  CompositionId composition;
  AdapterGeneration adapter_generation;
  CompositionGeneration composition_generation;
  ResidencyGeneration residency_generation;
  ArtifactGeneration artifact_generation;
  ModelRevisionId base_model_revision;
  CoordinatorEpoch epoch;
  WorkerId worker;
  WorkerBootId boot;
  AttemptId attempt;
  DeviceId device;
  NodeId node;
};

enum class AuthorityVerdict : std::uint8_t {
  authorized = 0,
  stale_worker,
  stale_boot,
  stale_attempt,
  stale_adapter_generation,
  stale_composition_generation,
  stale_residency_generation,
  stale_artifact_generation,
  stale_base_model,
  stale_epoch,
  unknown,
};

const char* to_string(AuthorityVerdict v) noexcept;

struct AuthorityCheck {
  AuthorityVerdict verdict = AuthorityVerdict::unknown;
  std::string reason;
  bool authorized() const noexcept { return verdict == AuthorityVerdict::authorized; }
};

// Deterministic fence validation against a reference authority snapshot.
AuthorityCheck validate_authority(const AuthorityFence& candidate, const AuthorityFence& reference);

}  // namespace adapter_fabric
