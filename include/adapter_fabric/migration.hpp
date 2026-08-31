// Adapter Fabric — governed adapter-state migration.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <string>
#include "adapter_fabric/identity.hpp"
#include "adapter_fabric/generation.hpp"

namespace adapter_fabric {

enum class MigrationPhase : std::uint8_t {
  not_started = 0,
  source_validated,
  destination_reserved,
  transferring,
  verifying,
  destination_ready,
  authority_promoted,
  source_retired,
  source_released,
  aborted,
};

const char* to_string(MigrationPhase p) noexcept;

struct MigrationPlan {
  AdapterInstanceId instance;
  AdapterId adapter;
  WorkerId source_worker;
  WorkerBootId source_boot;
  WorkerId dest_worker;
  WorkerBootId dest_boot;
  DeviceId dest_device;
  std::uint64_t bytes = 0;
  AdapterGeneration adapter_generation;
  ResidencyGeneration residency_generation;
  CoordinatorEpoch epoch;
};

// A validated, source-destructiveness-aware migration state machine. The
// source is not revoked before the destination is proven valid, unless the
// plan explicitly requires destructive movement.
class MigrationStateMachine {
 public:
  explicit MigrationStateMachine(bool destructive = false) : destructive_(destructive) {}

  MigrationPhase phase() const noexcept { return phase_; }

  // Advance; returns false if the requested transition is not allowed in the
  // current phase (deterministic, non-throwing).
  bool advance(MigrationPhase next);
  bool can_advance(MigrationPhase next) const noexcept;

  void reset() noexcept { phase_ = MigrationPhase::not_started; }

 private:
  MigrationPhase phase_ = MigrationPhase::not_started;
  bool destructive_ = false;
};

}  // namespace adapter_fabric
