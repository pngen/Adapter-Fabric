// Adapter Fabric — explicit adapter composition.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include "adapter_fabric/adapter.hpp"
#include "adapter_fabric/generation.hpp"
#include "adapter_fabric/identity.hpp"

namespace adapter_fabric {

// One member of a composition, frozen at publish time.
struct CompositionMember {
  AdapterId adapter_id;
  AdapterRevisionId revision;
  AdapterGeneration generation;
  std::uint64_t memory_bytes = 0;
  AdapterKind kind = AdapterKind::generic;
};

// An immutable published composition. Once built it must not be mutated;
// a changed composition gets a new generation.
struct Composition {
  CompositionId id;
  std::vector<CompositionMember> members;   // ordered
  CompositionGeneration generation;
  BaseModelId base_model;
  ModelRevisionId base_model_revision;
  std::string policy;
  std::uint64_t total_memory_bytes = 0;
  std::string digest;                     // digest of the authorized composition
  std::string validation_summary;
  bool is_valid = false;
};

// Validates membership and freezes a composition. Deterministic: identical
// inputs produce an identical digest.
class CompositionBuilder {
 public:
    CompositionBuilder(BaseModelId base, ModelRevisionId rev, std::string policy, CompositionGeneration gen = CompositionGeneration{});

  // Add a member. Stores the descriptor so base-model and conflict checks
  // can be performed against full metadata. Caller controls ordering.
  CompositionBuilder& add(const AdapterDescriptor& d);

  // Build and validate. Throws Error(invalid_composition) on any rejection.
  Composition build();

  void clear() { descriptors_.clear(); }

  static std::string compute_digest(const std::vector<CompositionMember>& members, std::string_view policy);

 private:
  BaseModelId base_;
  ModelRevisionId rev_;
  std::string policy_;
  CompositionGeneration gen_;
  std::vector<const AdapterDescriptor*> descriptors_;
  std::vector<CompositionMember> members_;
};

}  // namespace adapter_fabric
