// Adapter Fabric — deterministic PRNG.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <cstddef>
#include <array>

namespace adapter_fabric {

// splitmix64 — deterministic, seeding-friendly PRNG. Fixed-seed by default so
// tests are reproducible; seed() allows distinct streams.
class Rng {
 public:
  static Rng& instance() {
    static Rng rng{0x9E3779B97F4A7C15ULL};
    return rng;
  }

  explicit Rng(std::uint64_t seed) noexcept : state_(seed) {}

  void seed(std::uint64_t s) noexcept { state_ = s; }
  std::uint64_t seed() const noexcept { return state_; }

  std::uint64_t next() noexcept {
    std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }

  // Fill a byte buffer with pseudo-random octets.
  void fill(std::uint8_t* out, std::size_t n) noexcept {
    std::size_t i = 0;
    while (i < n) {
      const std::uint64_t v = next();
      const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
      const std::size_t take = (n - i) < sizeof(v) ? (n - i) : sizeof(v);
      for (std::size_t k = 0; k < take; ++k) out[i++] = p[k];
    }
  }

 private:
  std::uint64_t state_;
};

}  // namespace adapter_fabric
