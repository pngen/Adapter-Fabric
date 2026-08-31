// Adapter Fabric — composition implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/composition.hpp"
#include "adapter_fabric/checksum.hpp"
#include "adapter_fabric/error.hpp"
#include <set>

namespace adapter_fabric {

CompositionBuilder::CompositionBuilder(BaseModelId base, ModelRevisionId rev, std::string policy,
                                       CompositionGeneration gen)
    : base_(base), rev_(rev), policy_(std::move(policy)), gen_(gen) {}

CompositionBuilder& CompositionBuilder::add(const AdapterDescriptor& d) {
  for (const auto& x : members_) if (x.adapter_id == d.id) throw Error(ErrorCode::invalid_composition, "duplicate adapter member");
  if (d.generation.is_none()) throw Error(ErrorCode::invalid_composition, "member has no generation");
  if (d.base_model != base_) throw Error(ErrorCode::invalid_composition, "member targets a different base model");
  if (d.base_model_revision != rev_) throw Error(ErrorCode::invalid_composition, "member targets a different model revision");
  CompositionMember m;
  m.adapter_id = d.id;
  m.revision = d.revision;
  m.generation = d.generation;
  m.memory_bytes = d.memory_bytes;
  m.kind = d.kind;
  members_.push_back(m);
  descriptors_.push_back(&d);
  return *this;
}

std::string CompositionBuilder::compute_digest(const std::vector<CompositionMember>& members,
                                               std::string_view policy) {
  std::string canonical;
  for (const auto& m : members) {
    canonical += m.adapter_id.str(); canonical += '|';
    canonical += m.revision.str(); canonical += '|';
    canonical += std::to_string(m.generation.value()); canonical += '|';
    canonical += std::to_string(m.memory_bytes); canonical += '|';
    canonical += std::to_string(static_cast<int>(m.kind)); canonical += ';';
  }
  canonical += "@";
  canonical.append(policy);
  std::string out(8, '\0');
  const std::uint64_t h = fnv1a_64(canonical);
  for (int i = 0; i < 8; ++i) out[static_cast<std::size_t>(i)] = static_cast<char>((h >> (8 * i)) & 0xFF);
  return hex_encode(out);
}

Composition CompositionBuilder::build() {
  Composition c;
  c.id = CompositionId::generate();
  c.generation = gen_;
  c.base_model = base_;
  c.base_model_revision = rev_;
  c.policy = policy_;
  c.members = members_;

  if (members_.empty()) {
    throw Error(ErrorCode::invalid_composition, "composition has no members");
  }

  std::set<std::string> seen_ids;
  std::set<std::string> seen_modules;
  std::uint64_t total = 0;
  for (std::size_t i = 0; i < members_.size(); ++i) {
    const auto& m = members_[i];
    const auto& d = *descriptors_[i];
    const std::string idkey = m.adapter_id.str();
    if (!seen_ids.insert(idkey).second) {
      throw Error(ErrorCode::invalid_composition, "duplicate adapter member " + idkey);
    }
    if (m.generation.is_none()) {
      throw Error(ErrorCode::invalid_composition, "member has no generation");
    }
    if (d.base_model != base_) {
      throw Error(ErrorCode::invalid_composition, "member targets a different base model");
    }
    if (d.base_model_revision != rev_) {
      throw Error(ErrorCode::invalid_composition, "member targets a different model revision");
    }
    if (d.memory_bytes != m.memory_bytes) {
      throw Error(ErrorCode::invalid_composition, "member memory accounting mismatch");
    }
    total += m.memory_bytes;
    for (const auto& tgt : d.targets) {
      if (!seen_modules.insert(tgt.name).second) {
        throw Error(ErrorCode::invalid_composition, "conflicting target module " + tgt.name);
      }
    }
  }

  c.total_memory_bytes = total;
  c.digest = compute_digest(members_, policy_);
  c.validation_summary = "members=" + std::to_string(members_.size()) + ", digest=" + c.digest;
  c.is_valid = true;
  return c;
}

}  // namespace adapter_fabric