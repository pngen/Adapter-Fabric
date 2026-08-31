// Adapter Fabric — capacity accounting implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/capacity.hpp"

namespace adapter_fabric {

CapacityAccountant::CapacityAccountant(std::map<ResidencyLocation, std::uint64_t> limits)
    : limits_(std::move(limits)) {
  for (auto loc : {ResidencyLocation::device_memory, ResidencyLocation::pinned_host,
                   ResidencyLocation::pageable_host, ResidencyLocation::process_local,
                   ResidencyLocation::node_local, ResidencyLocation::persistent_storage}) {
    used_.emplace(loc, 0);
    limits_.emplace(loc, 0);
  }
}

void CapacityAccountant::reserve(ResidencyLocation loc, std::uint64_t bytes, const std::string& owner) {
  auto& held = held_[{loc, owner}];
  if (held != 0) {
    throw Error(ErrorCode::double_reservation, "double reservation for " + owner + " in " + to_string(loc));
  }
  if (bytes > limit(loc) - used(loc)) {
    throw Error(ErrorCode::capacity_overcommit, std::string("overcommit in ") + to_string(loc));
  }
  held = bytes;
  used_[loc] += bytes;
}

void CapacityAccountant::release(ResidencyLocation loc, std::uint64_t bytes, const std::string& owner) {
  auto& held = held_[{loc, owner}];
  if (held == 0) {
    throw Error(ErrorCode::double_release, "double release for " + owner + " in " + to_string(loc));
  }
  if (bytes > held) {
    throw Error(ErrorCode::capacity_underflow, "release exceeds reservation for " + owner + " in " + to_string(loc));
  }
  held -= bytes;
  used_[loc] -= bytes;
}

void CapacityAccountant::adjust(ResidencyLocation loc, std::int64_t delta, const std::string& owner) {
  if (delta >= 0) {
    reserve(loc, static_cast<std::uint64_t>(delta), owner);
  } else {
    release(loc, static_cast<std::uint64_t>(-delta), owner);
  }
}

std::uint64_t CapacityAccountant::used(ResidencyLocation loc) const {
  auto it = used_.find(loc);
  return it == used_.end() ? 0 : it->second;
}

std::uint64_t CapacityAccountant::limit(ResidencyLocation loc) const {
  auto it = limits_.find(loc);
  return it == limits_.end() ? 0 : it->second;
}

std::uint64_t CapacityAccountant::free(ResidencyLocation loc) const {
  return limit(loc) > used(loc) ? (limit(loc) - used(loc)) : 0;
}

bool CapacityAccountant::over_limit(ResidencyLocation loc) const { return used(loc) > limit(loc); }
bool CapacityAccountant::is_clean() const noexcept { for (const auto& kv : used_) if (kv.second != 0) return false; return true; }
std::map<ResidencyLocation, std::uint64_t> CapacityAccountant::usage() const { return used_; }
void CapacityAccountant::set_limit(ResidencyLocation loc, std::uint64_t bytes) { limits_[loc] = bytes; }

}  // namespace adapter_fabric
