// Adapter Fabric — identity tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "adapter_fabric/identity.hpp"
#include "adapter_fabric/uuid.hpp"
#include "adapter_fabric/rng.hpp"

using namespace adapter_fabric;

AF_TEST_CASE(identity_non_nil_and_roundtrip) {
  auto id = AdapterId::generate();
  AF_CHECK(!id.is_nil());
  std::string s = id.str();
  auto parsed = AdapterId::parse(s);
  bool ok = std::holds_alternative<AdapterId>(parsed);
  AF_CHECK(ok);
  if (ok) { AF_CHECK(std::get<AdapterId>(parsed) == id); }
  AF_CHECK(s.size() == 36);
}

AF_TEST_CASE(identity_deterministic_generation) {
  Rng a(42), b(42);
  auto u1 = Uuid::generate(a);
  auto u2 = Uuid::generate(b);
  AF_CHECK(u1 == u2);
  AF_CHECK(!u1.is_nil());
}

AF_TEST_CASE(identity_never_reuse_after_generation_change) {
  auto id1 = AdapterId::generate();
  auto id2 = AdapterId::generate();
  AF_CHECK(id1 != id2);
  AF_CHECK(!id1.is_nil() && !id2.is_nil());
}

AF_TEST_CASE(identity_nil_is_distinct) {
  auto n = AdapterId::nil();
  AF_CHECK(n.is_nil());
  AF_CHECK(AdapterId::parse(n.str()).index() == 0); // parses back to an Id
}

AF_TEST_CASE(uuid_parse_rejects_bad) {
  auto bad = Uuid::parse("not-a-uuid-no-way");
  AF_CHECK(std::holds_alternative<std::string>(bad));
  auto badhex = Uuid::parse("1234567890abcdef1234567890abcdef"); // 32 chars, no dashes
  AF_CHECK(std::holds_alternative<std::string>(badhex));
}

AF_TEST_MAIN
