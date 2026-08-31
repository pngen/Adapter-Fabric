// Adapter Fabric — lifecycle implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/lifecycle.hpp"

namespace adapter_fabric {

const char* to_string(LifecycleState s) noexcept {
  switch (s) {
    case LifecycleState::declared: return "declared";
    case LifecycleState::validating: return "validating";
    case LifecycleState::valid: return "valid";
    case LifecycleState::loading: return "loading";
    case LifecycleState::loaded: return "loaded";
    case LifecycleState::ready: return "ready";
    case LifecycleState::active: return "active";
    case LifecycleState::deactivating: return "deactivating";
    case LifecycleState::migrating: return "migrating";
    case LifecycleState::stale: return "stale";
    case LifecycleState::invalidated: return "invalidated";
    case LifecycleState::evicting: return "evicting";
    case LifecycleState::evicted: return "evicted";
    case LifecycleState::failed: return "failed";
    case LifecycleState::retired: return "retired";
  }
  return "unknown";
}

namespace {
constexpr std::uint8_t N = 255;
using L = LifecycleState;
std::uint8_t idx(LifecycleState s) noexcept { return static_cast<std::uint8_t>(s); }

// Allow-list row-major indexed by [from][to]. 'from' errors out to a short map.
bool allowed(LifecycleState from, LifecycleState to) noexcept {
  switch (from) {
    case L::declared:
      return to == L::declared || to == L::validating || to == L::evicting ||
             to == L::invalidated || to == L::retired || to == L::failed;
    case L::validating:
      return to == L::valid || to == L::declared || to == L::stale ||
             to == L::invalidated || to == L::retired || to == L::failed;
    case L::valid:
      return to == L::valid || to == L::validating || to == L::loading ||
             to == L::stale || to == L::invalidated || to == L::evicting ||
             to == L::evicted || to == L::retired || to == L::failed;
    case L::loading:
      return to == L::loaded || to == L::ready || to == L::stale ||
             to == L::invalidated || to == L::evicting || to == L::failed ||
             to == L::evicted;
    case L::loaded:
      return to == L::ready || to == L::loaded || to == L::loading ||
             to == L::stale || to == L::invalidated || to == L::evicting ||
             to == L::failed;
    case L::ready:
      return to == L::active || to == L::ready || to == L::loaded ||
             to == L::migrating || to == L::stale || to == L::invalidated ||
             to == L::evicting || to == L::failed;
    case L::active:
      return to == L::deactivating || to == L::migrating || to == L::stale ||
             to == L::invalidated || to == L::failed || to == L::evicting;
    case L::deactivating:
      return to == L::ready || to == L::active || to == L::stale ||
             to == L::invalidated || to == L::evicting || to == L::failed;
    case L::migrating:
      return to == L::ready || to == L::active || to == L::stale ||
             to == L::invalidated || to == L::evicting || to == L::failed;
    case L::stale:
      return to == L::validating || to == L::loading || to == L::evicting ||
             to == L::invalidated || to == L::failed || to == L::retired;
    case L::invalidated:
      return to == L::declared || to == L::loading || to == L::evicting ||
             to == L::retired || to == L::failed;
    case L::evicting:
      return to == L::evicted || to == L::invalidated || to == L::failed ||
             to == L::retired;
    case L::evicted:
      return to == L::declared || to == L::retired || to == L::failed;
    case L::failed:
      return to == L::declared || to == L::validating || to == L::retired ||
             to == L::evicting || to == L::invalidated;
    case L::retired:
      return to == L::retired;
  }
  return false;
}
}  // namespace

bool lifecycle_can_transition(LifecycleState from, LifecycleState to) noexcept {
  return allowed(from, to);
}

void lifecycle_validate_transition(LifecycleState from, LifecycleState to) {
  if (!allowed(from, to)) {
    throw Error(ErrorCode::invalid_state_transition,
                std::string("invalid lifecycle transition ") + to_string(from) + " -> " + to_string(to));
  }
}

void Lifecycle::transition_to(LifecycleState to) {
  lifecycle_validate_transition(state_, to);
  state_ = to;
}

}  // namespace adapter_fabric
