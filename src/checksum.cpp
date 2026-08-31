// Adapter Fabric — checksum/digest implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/checksum.hpp"

namespace adapter_fabric {

std::string hex_encode(const std::uint8_t* data, std::size_t len) {
  static const char* hex = "0123456789abcdef";
  std::string out;
  out.reserve(len * 2);
  for (std::size_t i = 0; i < len; ++i) {
    out.push_back(hex[data[i] >> 4]);
    out.push_back(hex[data[i] & 0x0F]);
  }
  return out;
}

std::string hex_encode(std::string_view bytes) {
  return hex_encode(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
}

}  // namespace adapter_fabric
