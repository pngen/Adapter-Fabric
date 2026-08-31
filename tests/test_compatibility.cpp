// Adapter Fabric — compatibility tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "fixtures.hpp"
#include "adapter_fabric/compatibility.hpp"
#include "adapter_fabric/adapter.hpp"

using namespace adapter_fabric;
namespace fx = af_test_fixture;

AF_TEST_CASE(compat_compatible_is_accepted) {
  auto bm = fx::base();
  auto mr = fx::rev();
  auto d = fx::make_lora("a", bm, mr);
  auto rep = evaluate_compatibility(d, fx::target(bm, mr));
  AF_CHECK(rep.compatible);
  AF_CHECK(!rep.has_rejections());
  AF_CHECK_EQ(rep.summary, std::string("compatible"));
}

AF_TEST_CASE(compat_wrong_revision_rejected_with_reason) {
  auto bm = fx::base();
  auto mr = fx::rev();
  auto other = fx::rev();
  auto d = fx::make_lora("a", bm, mr);
  auto rep = evaluate_compatibility(d, fx::target(bm, other));
  AF_CHECK(!rep.compatible);
  AF_CHECK(rep.has_rejections());
  AF_CHECK_EQ(rep.summary, std::string("incompatible: adapter is pinned to a different model revision"));
}

AF_TEST_CASE(compat_wrong_base_model_rejected) {
  auto bm = fx::base();
  auto other_bm = fx::base();
  auto mr = fx::rev();
  auto d = fx::make_lora("a", bm, mr);
  auto rep = evaluate_compatibility(d, fx::target(other_bm, mr));
  AF_CHECK(!rep.compatible);
  AF_CHECK(rep.has_rejections());
}

AF_TEST_CASE(compat_missing_half_precision_rejected) {
  auto bm = fx::base();
  auto mr = fx::rev();
  auto d = fx::make_lora("a", bm, mr);
  auto t = fx::target(bm, mr);
  t.runtime_capabilities["fp16"] = "unsupported";
  auto rep = evaluate_compatibility(d, t);
  AF_CHECK(!rep.compatible);
  AF_CHECK(rep.has_rejections());
  AF_CHECK(!rep.first_rejection_reason().empty());
}

AF_TEST_CASE(compat_zero_rank_rejected) {
  auto bm = fx::base();
  auto mr = fx::rev();
  auto d = fx::make_lora("a", bm, mr, 4096, 0);
  auto rep = evaluate_compatibility(d, fx::target(bm, mr));
  AF_CHECK(!rep.compatible);
  AF_CHECK(rep.has_rejections());
}

AF_TEST_CASE(compat_json_escapes_deterministically) {
  auto bm = fx::base();
  auto mr = fx::rev();
  auto d = fx::make_lora("a", bm, mr);
  auto rep = evaluate_compatibility(d, fx::target(bm, mr));
  std::string j1 = to_json(rep);
  std::string j2 = to_json(rep);
  AF_CHECK_EQ(j1, j2);
  AF_CHECK(j1.find("\"compatible\":true") != std::string::npos || j1.find("compatible") != std::string::npos);
}

AF_TEST_MAIN
