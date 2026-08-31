// Adapter Fabric — structured explainability.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "adapter_fabric/compatibility.hpp"
#include "adapter_fabric/authority.hpp"
#include "adapter_fabric/lifecycle.hpp"

namespace adapter_fabric {

enum class ExplainKind : std::uint8_t {
  compatibility = 0,
  readiness,
  activation,
  reuse,
  migration,
  eviction,
  composition,
  stale_authority,
  invalidation,
};

struct Explanation {
  ExplainKind kind;
  bool ok = false;
  std::string text;     // deterministic human text
  std::string json;     // deterministic JSON
  std::vector<std::string> reasons;
};

Explanation explain_compatibility(const CompatibilityReport& r);
Explanation explain_readiness(bool ready, std::string_view state, std::string_view reason);
Explanation explain_activation(bool ok, const AuthorityCheck& check);
Explanation explain_reuse(bool permitted, std::string_view reason);
Explanation explain_migration(bool selected, std::string_view reason);
Explanation explain_eviction(bool permitted, std::string_view reason);
Explanation explain_composition(bool valid, std::string_view summary);
Explanation explain_stale_authority(const AuthorityCheck& check);
Explanation explain_invalidation(std::string_view trigger, std::string_view reason);

// JSON rendering helpers.
std::string to_json(const CompatibilityReport& r);
std::string to_json(const AuthorityCheck& r);
std::string json_escape(std::string_view s);

}  // namespace adapter_fabric
