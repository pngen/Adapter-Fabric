// Adapter Fabric — adapter domain helpers.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/adapter.hpp"

namespace adapter_fabric {

const char* to_string(AdapterKind kind) noexcept {
  switch (kind) {
    case AdapterKind::generic: return "generic";
    case AdapterKind::lora: return "lora";
    case AdapterKind::full_finetune: return "full_finetune";
    case AdapterKind::prefix_tuning: return "prefix_tuning";
    case AdapterKind::custom: return "custom";
  }
  return "unknown";
}

const char* to_string(DType dtype) noexcept {
  switch (dtype) {
    case DType::unknown: return "unknown";
    case DType::f32: return "f32";
    case DType::f16: return "f16";
    case DType::bf16: return "bf16";
    case DType::f8_e4m3: return "f8_e4m3";
    case DType::f8_e5m2: return "f8_e5m2";
    case DType::int8: return "int8";
    case DType::int4: return "int4";
  }
  return "unknown";
}

std::size_t dtype_bytes(DType dtype) noexcept {
  switch (dtype) {
    case DType::unknown: return 0;
    case DType::f32: return 4;
    case DType::f16: return 2;
    case DType::bf16: return 2;
    case DType::f8_e4m3: return 1;
    case DType::f8_e5m2: return 1;
    case DType::int8: return 1;
    case DType::int4: return 1;
  }
  return 0;
}

const char* to_string(ValidationState v) noexcept {
  switch (v) {
    case ValidationState::none: return "none";
    case ValidationState::validating: return "validating";
    case ValidationState::valid: return "valid";
    case ValidationState::invalid: return "invalid";
  }
  return "unknown";
}

}  // namespace adapter_fabric
