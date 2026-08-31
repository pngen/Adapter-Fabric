// Adapter Fabric — deterministic property/invariant tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "fixtures.hpp"
#include "adapter_fabric/identity.hpp"
#include "adapter_fabric/generation.hpp"
#include "adapter_fabric/capacity.hpp"
#include "adapter_fabric/composition.hpp"

using namespace adapter_fabric;
namespace fx = af_test_fixture;

AF_TEST_CASE(property_identities_roundtrip_always_non_nil) {
  Rng rng(0xC0FFEE);
  for (int i = 0; i < 2000; ++i) {
    auto id = AdapterId::generate(rng);
    AF_CHECK(!id.is_nil());
    auto p = AdapterId::parse(id.str());
    AF_CHECK(std::holds_alternative<AdapterId>(p));
    if (std::holds_alternative<AdapterId>(p)) AF_CHECK(std::get<AdapterId>(p) == id);
  }
}

AF_TEST_CASE(property_generation_monotonic) {
  AdapterGeneration g = AdapterGeneration{1};
  for (int i = 0; i < 5000; ++i) { auto n = g.next(); AF_CHECK(n > g); AF_CHECK(!n.is_none()); g = n; }
}

AF_TEST_CASE(property_capacity_returns_to_baseline) {
  std::map<ResidencyLocation, std::uint64_t> lim{{ResidencyLocation::device_memory, 1u<<20}};
  CapacityAccountant cap(lim);
  Rng rng(7);
  for (int i = 0; i < 2000; ++i) {
    std::uint64_t b = 1 + (rng.next() % 4096);
    cap.reserve(ResidencyLocation::device_memory, b, "owner" + std::to_string(i));
    cap.release(ResidencyLocation::device_memory, b, "owner" + std::to_string(i));
  }
  AF_CHECK(cap.is_clean());
  AF_CHECK_EQ(cap.used(ResidencyLocation::device_memory), 0);
}

AF_TEST_CASE(property_composition_digest_idempotent) {
  auto bm = fx::base(); auto mr = fx::rev();
  auto d1 = fx::make_lora("a", bm, mr);
  auto d2 = fx::make_lora("b", bm, mr); d2.targets[0].name = "v_proj";
  std::string last;
  for (int i = 0; i < 50; ++i) {
    CompositionBuilder b(bm, mr, "policy", CompositionGeneration{1});
    b.add(d1); b.add(d2);
    Composition c = b.build();
    std::string digest = c.digest;
    if (i == 0) last = digest;
    else AF_CHECK_EQ(digest, last);   // identical members+policy (same ids) -> identical digest
  }
}

AF_TEST_MAIN