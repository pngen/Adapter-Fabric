// Adapter Fabric — exact capacity accounting.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include "adapter_fabric/error.hpp"
#include "adapter_fabric/residency.hpp"

namespace adapter_fabric {

// Tracks reservations and releases against per-location byte limits. Proves:
//   no overcommit, no double reservation, no double release, no leak, and
//   exact return-to-baseline after teardown.
class CapacityAccountant {
 public:
  CapacityAccountant() = default;
  explicit CapacityAccountant(std::map<ResidencyLocation, std::uint64_t> limits);

  // Reserve `bytes` in `loc` on behalf of `owner`. Rejects overcommit and
  // double reservation (same owner already holds `bytes` in `loc`).
  void reserve(ResidencyLocation loc, std::uint64_t bytes, const std::string& owner);

  // Release `bytes` in `loc` for `owner`. Rejects underflow and double release.
  void release(ResidencyLocation loc, std::uint64_t bytes, const std::string& owner);

  // Change the byte count for `owner` in `loc` by delta (reserve if positive,
  // release if negative). Used by the transactional loader.
  void adjust(ResidencyLocation loc, std::int64_t delta, const std::string& owner);

  std::uint64_t used(ResidencyLocation loc) const;
  std::uint64_t limit(ResidencyLocation loc) const;
  std::uint64_t free(ResidencyLocation loc) const;

  bool over_limit(ResidencyLocation loc) const;
  bool is_clean() const noexcept;   // all owners released, used == 0

  std::map<ResidencyLocation, std::uint64_t> usage() const;

  void set_limit(ResidencyLocation loc, std::uint64_t bytes);

 private:
  std::map<ResidencyLocation, std::uint64_t> used_;
  std::map<ResidencyLocation, std::uint64_t> limits_;
  // (loc, owner) -> bytes, to detect double reservation / double release.
  std::map<std::pair<ResidencyLocation, std::string>, std::uint64_t> held_;
};

}  // namespace adapter_fabric
