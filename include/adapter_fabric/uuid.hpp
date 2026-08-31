// Adapter Fabric — 128-bit UUID.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <variant>
#include "adapter_fabric/rng.hpp"

namespace adapter_fabric {

// A canonical 128-bit identifier (RFC-4122 byte layout). It is an opaque,
// value-semantic token with deterministic byte round-tripping and a stable
// canonical textual form (8-4-4-4-12 lowercase hex). Null UUID is all-zero.
//
// Identities must never silently become null: generators always return a fresh
// non-null value, and null is a distinct, explicit sentinel.
class Uuid {
 public:
  constexpr Uuid() noexcept : bytes_{} {}

  constexpr explicit Uuid(const std::array<std::uint8_t, 16>& b) noexcept
      : bytes_(b) {}

  static Uuid nil() noexcept { return Uuid{}; }
  bool is_nil() const noexcept {
    for (std::uint8_t b : bytes_) {
      if (b != 0) return false;
    }
    return true;
  }

  std::string str() const;
  static std::variant<Uuid, std::string> parse(std::string_view s) noexcept;
  static Uuid generate(Rng& rng) noexcept {
    Uuid u;
    rng.fill(u.bytes_.data(), u.bytes_.size());
    u.bytes_[6] = static_cast<std::uint8_t>((u.bytes_[6] & 0x0F) | 0x40);  // version 4
    u.bytes_[8] = static_cast<std::uint8_t>((u.bytes_[8] & 0x3F) | 0x80);  // variant
    return u;
  }
  static Uuid generate() noexcept { return generate(Rng::instance()); }

  const std::array<std::uint8_t, 16>& bytes() const noexcept { return bytes_; }
  std::array<std::uint8_t, 16>& bytes() noexcept { return bytes_; }

  friend bool operator==(const Uuid& a, const Uuid& b) noexcept { return a.bytes_ == b.bytes_; }
  friend bool operator!=(const Uuid& a, const Uuid& b) noexcept { return !(a == b); }
  friend bool operator<(const Uuid& a, const Uuid& b) noexcept {
    return a.bytes_ < b.bytes_;
  }

 private:
  std::array<std::uint8_t, 16> bytes_;
};

struct UuidHash {
  std::size_t operator()(const Uuid& u) const noexcept {
    const auto& b = u.bytes();
    std::size_t h = 1469598103934665603ULL;
    for (std::uint8_t x : b) {
      h ^= static_cast<std::size_t>(x);
      h *= 1099511628211ULL;
    }
    return h;
  }
};

}  // namespace adapter_fabric

namespace std {
template <>
struct hash<adapter_fabric::Uuid> {
  size_t operator()(const adapter_fabric::Uuid& u) const noexcept {
    return adapter_fabric::UuidHash{}(u);
  }
};
}  // namespace std
