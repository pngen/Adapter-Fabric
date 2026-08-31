// Adapter Fabric — migration implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/migration.hpp"

namespace adapter_fabric {

const char* to_string(MigrationPhase p) noexcept {
  switch (p) {
    case MigrationPhase::not_started: return "not_started";
    case MigrationPhase::source_validated: return "source_validated";
    case MigrationPhase::destination_reserved: return "destination_reserved";
    case MigrationPhase::transferring: return "transferring";
    case MigrationPhase::verifying: return "verifying";
    case MigrationPhase::destination_ready: return "destination_ready";
    case MigrationPhase::authority_promoted: return "authority_promoted";
    case MigrationPhase::source_retired: return "source_retired";
    case MigrationPhase::source_released: return "source_released";
    case MigrationPhase::aborted: return "aborted";
  }
  return "unknown";
}

bool MigrationStateMachine::can_advance(MigrationPhase next) const noexcept {
  using P = MigrationPhase;
  auto is_terminal = [](P p) { return p == P::aborted || p == P::source_released; };
  if (is_terminal(phase_)) return false;
  switch (phase_) {
    case P::not_started: return next == P::source_validated || next == P::aborted;
    case P::source_validated: return next == P::destination_reserved || next == P::aborted;
    case P::destination_reserved: return next == P::transferring || next == P::aborted;
    case P::transferring: return next == P::verifying || next == P::aborted;
    case P::verifying: return next == P::destination_ready || next == P::aborted ||
                            (destructive_ && next == P::authority_promoted);
    case P::destination_ready: return next == P::authority_promoted || next == P::aborted;
    case P::authority_promoted: return next == P::source_retired || next == P::aborted;
    case P::source_retired: return next == P::source_released || next == P::aborted;
    case P::aborted: return false;
    case P::source_released: return false;
  }
  return false;
}

bool MigrationStateMachine::advance(MigrationPhase next) {
  if (!can_advance(next)) return false;
  phase_ = next;
  return true;
}

}  // namespace adapter_fabric
