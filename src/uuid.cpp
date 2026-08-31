// Adapter Fabric — UUID implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/uuid.hpp"
#include <cstdio>

namespace adapter_fabric {

namespace {
int nibble(char c) noexcept {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
}  // namespace

std::string Uuid::str() const {
  char buf[37];
  static const char* hex = "0123456789abcdef";
  std::size_t i = 0;
  for (std::size_t k = 0; k < 16; ++k) {
    if (k == 4 || k == 6 || k == 8 || k == 10) buf[i++] = '-';
    buf[i++] = hex[bytes_[k] >> 4];
    buf[i++] = hex[bytes_[k] & 0x0F];
  }
  buf[i] = 0;
  return std::string(buf, 36);
}

std::variant<Uuid, std::string> Uuid::parse(std::string_view s) noexcept {
  if (s.size() != 36) return std::string("expected a 36-character UUID");
  std::array<std::uint8_t, 16> out{};
  std::size_t di = 0;
  for (std::size_t i = 0; i < s.size();) {
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (s[i] != '-') return std::string("expected a dash at position " + std::to_string(i));
      ++i;
      continue;
    }
    const int hi = nibble(s[i]);
    const int lo = nibble(s[i + 1]);
    if (hi < 0 || lo < 0) return std::string("invalid hex digit");
    out[di++] = static_cast<std::uint8_t>((hi << 4) | lo);
    i += 2;
  }
  return Uuid{out};
}

}  // namespace adapter_fabric
