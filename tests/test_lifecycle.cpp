// Adapter Fabric — lifecycle tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "adapter_fabric/lifecycle.hpp"

using namespace adapter_fabric;

AF_TEST_CASE(lifecycle_hashmap_transitions) {
  Lifecycle lc;
  AF_CHECK(lc.state() == LifecycleState::declared);
  lc.transition_to(LifecycleState::validating);
  lc.transition_to(LifecycleState::valid);
  lc.transition_to(LifecycleState::loading);
  lc.transition_to(LifecycleState::loaded);
  lc.transition_to(LifecycleState::ready);
  lc.transition_to(LifecycleState::active);
  lc.transition_to(LifecycleState::deactivating);
  lc.transition_to(LifecycleState::ready);
  AF_CHECK(lc.state() == LifecycleState::ready);
}

AF_TEST_CASE(lifecycle_invalid_transition_rejected) {
  Lifecycle lc;
  AF_CHECK_THROWS(lc.transition_to(LifecycleState::active));   // declared -> active invalid
  AF_CHECK(lc.state() == LifecycleState::declared);             // unchanged
  lc.transition_to(LifecycleState::validating);
  AF_CHECK_THROWS(lc.transition_to(LifecycleState::active));    // validating -> active invalid
  AF_CHECK(lc.state() == LifecycleState::validating);
}

AF_TEST_CASE(lifecycle_deterministic_table) {
  AF_CHECK(lifecycle_can_transition(LifecycleState::ready, LifecycleState::active));
  AF_CHECK(!lifecycle_can_transition(LifecycleState::active, LifecycleState::declared));
  AF_CHECK(lifecycle_can_transition(LifecycleState::active, LifecycleState::deactivating));
  AF_CHECK(lifecycle_can_transition(LifecycleState::stale, LifecycleState::validating));
  AF_CHECK(lifecycle_can_transition(LifecycleState::invalidated, LifecycleState::declared));
  AF_CHECK(!lifecycle_can_transition(LifecycleState::retired, LifecycleState::active));
}

AF_TEST_MAIN
