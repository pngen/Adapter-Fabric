// Adapter Fabric — adapter residency tracking.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "adapter_fabric/adapter.hpp"
#include "adapter_fabric/generation.hpp"
#include "adapter_fabric/identity.hpp"

namespace adapter_fabric {

enum class ResidencyLocation : std::uint8_t {
  device_memory = 0,
  pinned_host,
  pageable_host,
  process_local,
  node_local,
  persistent_storage,
};

const char* to_string(ResidencyLocation loc) noexcept;

enum class MigrationState : std::uint8_t {
  none = 0,
  target_reserved,
  transferring,
  verifying,
  dest_ready,
  migrating,
  source_retiring,
  source_released,
};

const char* to_string(MigrationState m) noexcept;

// Worker-local / device-local embodiment: an adapter instance binder. It
// belongs to a (worker, boot, device). Its authority is derived from the
// canonical descriptor plus the binding fence.
struct ResidencyRecord {
  AdapterInstanceId instance;
  AdapterId adapter_id;
  AdapterGeneration adapter_generation;
  ResidencyGeneration residency_generation;
  ResidencyLocation location = ResidencyLocation::process_local;
  std::uint64_t bytes = 0;
  WorkerId worker;
  WorkerBootId boot;
  CoordinatorEpoch epoch;
  DeviceId device;
  bool ready = false;
  std::uint32_t active_refs = 0;
  std::uint32_t pin_count = 0;
  bool protected_ = false;
  MigrationState migration = MigrationState::none;
  std::string freshness;
  std::string compat_summary;
  bool has_compat_evidence = false;
};

}  // namespace adapter_fabric
