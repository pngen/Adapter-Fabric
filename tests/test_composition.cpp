// Adapter Fabric — composition tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "fixtures.hpp"
#include "adapter_fabric/composition.hpp"
#include "adapter_fabric/fabric.hpp"
#include <vector>

using namespace adapter_fabric;
namespace fx = af_test_fixture;

AF_TEST_CASE(compose_valid_is_deterministic) {
  auto bm = fx::base();
  auto mr = fx::rev();
  auto d1 = fx::make_lora("a", bm, mr);
  auto d2 = fx::make_lora("b", bm, mr);
  d2.targets[0].name = "v_proj";   // avoid target conflict
  d2.memory_bytes = 2048;
  CompositionBuilder b(bm, mr, "policy-x", CompositionGeneration{1});
  b.add(d1); b.add(d2);
  Composition c = b.build();
  AF_CHECK(c.is_valid);
  AF_CHECK_EQ(c.members.size(), std::size_t(2));
  AF_CHECK_EQ(c.total_memory_bytes, d1.memory_bytes + d2.memory_bytes);
  // digest deterministic for same membership+policy.
  CompositionBuilder b2(bm, mr, "policy-x", CompositionGeneration{1});
  b2.add(d1); b2.add(d2);
  Composition c2 = b2.build();
  AF_CHECK_EQ(c.digest, c2.digest);
}

AF_TEST_CASE(compose_duplicate_member_rejected) {
  auto bm = fx::base();
  auto mr = fx::rev();
  auto d1 = fx::make_lora("a", bm, mr);
  CompositionBuilder b(bm, mr, "p", CompositionGeneration{1});
  b.add(d1);
  AF_CHECK_THROWS(b.add(d1));
}

AF_TEST_CASE(compose_base_model_mismatch_rejected) {
  auto bm = fx::base();
  auto mr = fx::rev();
  auto other = fx::base();
  auto d1 = fx::make_lora("a", bm, mr);
  auto d2 = fx::make_lora("b", other, mr);
  d2.targets[0].name = "v_proj";
  CompositionBuilder b(bm, mr, "p", CompositionGeneration{1});
  b.add(d1);
  AF_CHECK_THROWS(b.add(d2));
}

AF_TEST_CASE(compose_conflicting_target_module_rejected) {
  auto bm = fx::base();
  auto mr = fx::rev();
  auto d1 = fx::make_lora("a", bm, mr);   // targets "q_proj"
  auto d2 = fx::make_lora("b", bm, mr);   // also targets "q_proj"
  CompositionBuilder b(bm, mr, "p", CompositionGeneration{1});
  b.add(d1); b.add(d2);
  AF_CHECK_THROWS(b.build());
}

AF_TEST_CASE(compose_empty_rejected) {
  auto bm = fx::base();
  auto mr = fx::rev();
  CompositionBuilder b(bm, mr, "p", CompositionGeneration{1});
  AF_CHECK_THROWS(b.build());
}

AF_TEST_MAIN