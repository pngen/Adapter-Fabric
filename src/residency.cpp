// Adapter Fabric — residency helpers.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/residency.hpp"

namespace adapter_fabric {

const char* to_string(ResidencyLocation loc) noexcept {
  switch (loc) {
    case ResidencyLocation::device_memory: return "device_memory";
    case ResidencyLocation::pinned_host: return "pinned_host";
    case ResidencyLocation::pageable_host: return "pageable_host";
    case ResidencyLocation::process_local: return "process_local";
    case ResidencyLocation::node_local: return "node_local";
    case ResidencyLocation::persistent_storage: return "persistent_storage";
  }
  return "unknown";
}

const char* to_string(MigrationState m) noexcept {
  switch (m) {
    case MigrationState::none: return "none";
    case MigrationState::target_reserved: return "target_reserved";
    case MigrationState::transferring: return "transferring";
    case MigrationState::verifying: return "verifying";
    case MigrationState::dest_ready: return "dest_ready";
    case MigrationState::migrating: return "migrating";
    case MigrationState::source_retiring: return "source_retiring";
    case MigrationState::source_released: return "source_released";
  }
  return "unknown";
}

}  // namespace adapter_fabric
