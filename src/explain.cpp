// Adapter Fabric — explainability implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/explain.hpp"

namespace adapter_fabric {

std::string json_escape(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

std::string to_json(const CompatibilityReport& r) {
  std::string j = std::string("{\"compatible\":") + (r.compatible ? "true" : "false") + ",\"summary\":\"" + json_escape(r.summary) + "\",\"factors\":[";
  bool first = true;
  for (const auto& f : r.factors) {
    if (!first) j += ",";
    first = false;
    const char* status = f.status == FactorStatus::accepted ? "accepted" : (f.status == FactorStatus::rejected ? "rejected" : "missing");
    j += std::string("{\"name\":\"") + json_escape(f.name) + "\",\"status\":\"" + status + "\",\"detail\":\"" + json_escape(f.detail) + "\",\"reason\":\"" + json_escape(f.reason) + "\"}";
  }
  j += "]}";
  return j;
}

std::string to_json(const AuthorityCheck& r) {
  return std::string("{\"authorized\":") + (r.authorized() ? "true" : "false") + ",\"verdict\":\"" + to_string(r.verdict) + "\",\"reason\":\"" + json_escape(r.reason) + "\"}";
}

namespace {
Explanation make(ExplainKind k, bool ok, std::string text, std::vector<std::string> reasons) {
  Explanation e;
  e.kind = k;
  e.ok = ok;
  e.text = std::move(text);
  e.reasons = std::move(reasons);
  std::string j = std::string("{\"kind\":\"") + std::to_string(static_cast<int>(k)) + "\",\"ok\":" + (ok ? "true" : "false") + ",\"text\":\"" + json_escape(e.text) + "\",\"reasons\":[";
  for (std::size_t i = 0; i < e.reasons.size(); ++i) {
    if (i) j += ",";
    j += "\"" + json_escape(e.reasons[i]) + "\"";
  }
  j += "]}";
  e.json = std::move(j);
  return e;
}
}  // namespace

Explanation explain_compatibility(const CompatibilityReport& r) {
  std::vector<std::string> reasons;
  for (const auto& f : r.factors) {
    if (f.status == FactorStatus::rejected) reasons.push_back(f.name + ": " + f.reason);
    else if (f.status == FactorStatus::missing) reasons.push_back(f.name + ": missing evidence");
  }
  return make(ExplainKind::compatibility, r.compatible, r.summary, std::move(reasons));
}

Explanation explain_readiness(bool ready, std::string_view state, std::string_view reason) {
  std::string text = std::string("state=") + std::string(state) + (ready ? ", ok=true" : ", ok=false") + ", reason=" + std::string(reason);
  return make(ExplainKind::readiness, ready, std::move(text), {std::string(reason)});
}

Explanation explain_activation(bool ok, const AuthorityCheck& check) {
  std::string text = std::string("activation ") + (ok ? "granted" : "denied") + ": " + check.reason;
  return make(ExplainKind::activation, ok && check.authorized(), std::move(text), {check.reason});
}

Explanation explain_reuse(bool permitted, std::string_view reason) {
  std::string text = std::string("reuse ") + (permitted ? "permitted" : "denied") + ": " + std::string(reason);
  return make(ExplainKind::reuse, permitted, std::move(text), {std::string(reason)});
}

Explanation explain_migration(bool selected, std::string_view reason) {
  std::string text = std::string("migration ") + (selected ? "selected" : "not selected") + ": " + std::string(reason);
  return make(ExplainKind::migration, selected, std::move(text), {std::string(reason)});
}

Explanation explain_eviction(bool permitted, std::string_view reason) {
  std::string text = std::string("eviction ") + (permitted ? "permitted" : "denied") + ": " + std::string(reason);
  return make(ExplainKind::eviction, permitted, std::move(text), {std::string(reason)});
}

Explanation explain_composition(bool valid, std::string_view summary) {
  std::string text = std::string("composition ") + (valid ? "valid" : "invalid") + ": " + std::string(summary);
  return make(ExplainKind::composition, valid, std::move(text), {std::string(summary)});
}

Explanation explain_stale_authority(const AuthorityCheck& check) {
  std::string text = std::string("authority ") + (check.authorized() ? "current" : "stale") + ": " + check.reason;
  return make(ExplainKind::stale_authority, check.authorized(), std::move(text), {check.reason});
}

Explanation explain_invalidation(std::string_view trigger, std::string_view reason) {
  std::string text = std::string("invalidated by ") + std::string(trigger) + ": " + std::string(reason);
  return make(ExplainKind::invalidation, false, std::move(text), {std::string(reason)});
}

}  // namespace adapter_fabric
