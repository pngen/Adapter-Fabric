// Adapter Fabric — error model.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>

namespace adapter_fabric {

// Coarse error categories. Every operation that can fail reports one of these,
// usually alongside a specific reason string and structured context.
enum class ErrorCode : std::uint16_t {
  ok = 0,
  invalid_identity,
  invalid_generation,
  invalid_state_transition,
  incompatibility,
  invalid_composition,
  capacity_overcommit,
  capacity_underflow,
  double_reservation,
  double_release,
  not_found,
  already_exists,
  checksum_mismatch,
  malformed,
  truncation,
  duplicate,
  invalid_enum,
  invalid_generation_value,
  integer_overflow,
  trailing_garbage,
  unsupported_version,
  corrupt,
  stale_authority,
  stale_worker,
  stale_boot,
  stale_attempt,
  stale_generation,
  invalid_activation,
  eviction_denied,
  invalid_migration,
  invalid_invalidation,
  io_error,
  protocol_error,
  transport_error,
  resource_unavailable,
  not_available,
  invalid_argument,
  precondition_error,
  authority_conflict,
  internal_error,
};

const char* to_string(ErrorCode code) noexcept;

// An exception carrying a machine-readable code and a human message plus a
// free-form context string that structured reporters can surface.
class Error : public std::runtime_error {
 public:
  Error(ErrorCode code, std::string message, std::string context = {});
  ErrorCode code() const noexcept { return code_; }
  const std::string& context() const noexcept { return context_; }

 private:
  ErrorCode code_;
  std::string context_;
};

#define AF_THROW(code, message)   throw ::adapter_fabric::Error((code), (message), "")

#define AF_THROW_C(code, message, ctx)   throw ::adapter_fabric::Error((code), (message), (ctx))

}  // namespace adapter_fabric
