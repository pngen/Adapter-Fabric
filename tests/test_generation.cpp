// Adapter Fabric — generation tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "adapter_fabric/generation.hpp"

using namespace adapter_fabric;

AF_TEST_CASE(generation_starts_none) {
  AdapterGeneration g;
  AF_CHECK(g.is_none());
  AF_CHECK(g.value() == 0);
}

AF_TEST_CASE(generation_next_never_returns_to_none) {
  AdapterGeneration g;
  auto g1 = g.next();
  AF_CHECK(g1.value() == 1);
  AF_CHECK(!g1.is_none());
  // Repeated next is monotonic and never wraps to 0.
  AdapterGeneration prev = g1;
  for (int i = 0; i < 1000; ++i) {
    auto n = prev.next();
    AF_CHECK(n > prev);
    AF_CHECK(!n.is_none());
    prev = n;
  }
}

AF_TEST_CASE(generation_wraps_max_but_not_to_zero) {
  AdapterGeneration g{std::numeric_limits<std::uint64_t>::max()};
  auto n = g.next();
  AF_CHECK(n.value() == std::numeric_limits<std::uint64_t>::max());
  AF_CHECK(!n.is_none());
}

AF_TEST_MAIN
