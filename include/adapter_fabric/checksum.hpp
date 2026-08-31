// Adapter Fabric — content digests and checksums.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace adapter_fabric {

// FNV-1a 64-bit. Deterministic, cheap, and used both for artifact digests
// and for byte-range checksums in the persistence layer.
constexpr std::uint64_t fnv1a_64(std::uint64_t hash, const void* data, std::size_t len) noexcept {
  const auto* p = static_cast<const std::uint8_t*>(data);
  for (std::size_t i = 0; i < len; ++i) {
    hash ^= static_cast<std::uint64_t>(p[i]);
    hash *= 1099511628211ULL;
  }
  return hash;
}

inline std::uint64_t fnv1a_64(std::string_view s) noexcept {
  return fnv1a_64(1469598103934665603ULL, s.data(), s.size());
}

// Hex-encode a byte buffer (lowercase).
std::string hex_encode(const std::uint8_t* data, std::size_t len);
std::string hex_encode(std::string_view bytes);

// Compute a hex digest of a string payload in a stable, versioned way.
inline std::string content_digest(std::string_view bytes) {
  std::uint64_t h = fnv1a_64(bytes);
  std::string out(8, '\0');
  for (int i = 0; i < 8; ++i) out[static_cast<std::size_t>(i)] = static_cast<char>((h >> (8 * i)) & 0xFF);
  return hex_encode(out);
}

}  // namespace adapter_fabric
