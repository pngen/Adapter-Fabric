// Adapter Fabric — guarded adapter lifecycle.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <string>
#include "adapter_fabric/error.hpp"

namespace adapter_fabric {

// The guarded lifecycle. ACTIVE means execution authority exists, not merely
// that bytes are resident. Every transition is validated against an explicit
// allow-list; invalid transitions fail deterministically.
enum class LifecycleState : std::uint8_t {
  declared = 0,
  validating,
  valid,
  loading,
  loaded,
  ready,
  active,
  deactivating,
  migrating,
  stale,
  invalidated,
  evicting,
  evicted,
  failed,
  retired,
};

const char* to_string(LifecycleState state) noexcept;

// Deterministic transition table. Returns true iff the transition is allowed.
bool lifecycle_can_transition(LifecycleState from, LifecycleState to) noexcept;

// Validate a transition; throws Error(invalid_state_transition) on rejection.
void lifecycle_validate_transition(LifecycleState from, LifecycleState to);

class Lifecycle {
 public:
  Lifecycle() noexcept = default;
  explicit Lifecycle(LifecycleState s) noexcept : state_(s) {}

  LifecycleState state() const noexcept { return state_; }

  // Transition to `to`, validating against the allow-list. On rejection a
  // deterministic Error is thrown and the current state is unchanged.
  void transition_to(LifecycleState to);

  bool is_terminal() const noexcept {
    return state_ == LifecycleState::evicted ||
           state_ == LifecycleState::retired ||
           state_ == LifecycleState::failed;
  }

  void set_state(LifecycleState s) noexcept { state_ = s; }

 private:
  LifecycleState state_ = LifecycleState::declared;
};

}  // namespace adapter_fabric
