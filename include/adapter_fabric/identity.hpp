// Adapter Fabric — strongly-typed domain identities.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <variant>
#include "adapter_fabric/uuid.hpp"

namespace adapter_fabric {

// A strongly-typed 128-bit identity. Each Tag produces a distinct C++ type, so
// an AdapterId can never silently be assigned to a CompositionId. Every identity
// value is backed by a Uuid and round-trips deterministically.
template <typename Tag>
class Id {
 public:
  constexpr Id() noexcept : u_() {}
  constexpr explicit Id(const Uuid& u) noexcept : u_(u) {}

  static Id generate() noexcept { return Id{Uuid::generate()}; }
  static Id generate(Rng& rng) noexcept { return Id{Uuid::generate(rng)}; }
  static Id nil() noexcept { return Id{Uuid::nil()}; }

  static std::variant<Id, std::string> parse(std::string_view s) noexcept {
    auto p = Uuid::parse(s);
    if (auto* u = std::get_if<Uuid>(&p)) return Id{*u};
    return std::get<std::string>(p);
  }

  bool is_nil() const noexcept { return u_.is_nil(); }
  const Uuid& uuid() const noexcept { return u_; }
  std::string str() const { return u_.str(); }

  friend bool operator==(const Id& a, const Id& b) noexcept { return a.u_ == b.u_; }
  friend bool operator!=(const Id& a, const Id& b) noexcept { return a.u_ != b.u_; }
  friend bool operator<(const Id& a, const Id& b) noexcept { return a.u_ < b.u_; }

 private:
  Uuid u_;
};

namespace idtag {
struct adapter_id             {};
struct adapter_revision_id    {};
struct adapter_artifact_id    {};
struct adapter_instance_id    {};
struct adapter_set_id         {};
struct composition_id         {};
struct base_model_id          {};
struct model_revision_id      {};
struct worker_id              {};
struct worker_boot_id         {};
struct node_id                {};
struct device_id              {};
struct attempt_id             {};
}  // namespace idtag

using AdapterId          = Id<idtag::adapter_id>;
using AdapterRevisionId  = Id<idtag::adapter_revision_id>;
using AdapterArtifactId  = Id<idtag::adapter_artifact_id>;
using AdapterInstanceId  = Id<idtag::adapter_instance_id>;
using AdapterSetId       = Id<idtag::adapter_set_id>;
using CompositionId      = Id<idtag::composition_id>;
using BaseModelId        = Id<idtag::base_model_id>;
using ModelRevisionId    = Id<idtag::model_revision_id>;
using WorkerId           = Id<idtag::worker_id>;
using WorkerBootId       = Id<idtag::worker_boot_id>;
using NodeId             = Id<idtag::node_id>;
using DeviceId           = Id<idtag::device_id>;
using AttemptId          = Id<idtag::attempt_id>;

}  // namespace adapter_fabric

template <typename Tag>
struct std::hash<adapter_fabric::Id<Tag>> {
  std::size_t operator()(const adapter_fabric::Id<Tag>& id) const noexcept {
    return adapter_fabric::UuidHash{}(id.uuid());
  }
};
