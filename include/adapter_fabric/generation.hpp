// Adapter Fabric — strongly-typed monotonic generations.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <limits>

namespace adapter_fabric {

// A monotonically increasing generation. Distinct tags give distinct types so a
// ResidencyGeneration can never be compared to an AdapterGeneration. A
// generation zero is explicitly "none" — no generation exists yet; it is not a
// valid "current" generation.
template <typename Tag>
class Generation {
 public:
  constexpr Generation() noexcept : value_(0) {}
  constexpr explicit Generation(std::uint64_t v) noexcept : value_(v) {}

  static constexpr Generation none() noexcept { return Generation{}; }
  bool is_none() const noexcept { return value_ == 0; }

  std::uint64_t value() const noexcept { return value_; }

  // Advance to the next generation. Wraps none (0) -> 1. Never returns to none.
  Generation next() const noexcept {
    const std::uint64_t v = (value_ == std::numeric_limits<std::uint64_t>::max())
                               ? value_
                               : value_ + 1;
    return Generation{v == 0 ? 1 : v};
  }

  friend bool operator==(Generation a, Generation b) noexcept { return a.value_ == b.value_; }
  friend bool operator!=(Generation a, Generation b) noexcept { return a.value_ != b.value_; }
  friend bool operator<(Generation a, Generation b) noexcept { return a.value_ < b.value_; }
  friend bool operator<=(Generation a, Generation b) noexcept { return a.value_ <= b.value_; }
  friend bool operator>(Generation a, Generation b) noexcept { return a.value_ > b.value_; }
  friend bool operator>=(Generation a, Generation b) noexcept { return a.value_ >= b.value_; }

 private:
  std::uint64_t value_;
};

// Generation tags.
namespace gen {
struct adapter           {};
struct residency         {};
struct composition       {};
struct artifact          {};
struct authority         {};
struct coordinator_epoch {};
struct policy            {};
}  // namespace gen

using AdapterGeneration      = Generation<gen::adapter>;
using ResidencyGeneration    = Generation<gen::residency>;
using CompositionGeneration  = Generation<gen::composition>;
using ArtifactGeneration     = Generation<gen::artifact>;
using AuthorityGeneration    = Generation<gen::authority>;
using CoordinatorEpoch       = Generation<gen::coordinator_epoch>;
using PolicyGeneration       = Generation<gen::policy>;

}  // namespace adapter_fabric
