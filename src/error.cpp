// Adapter Fabric — error implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/error.hpp"

namespace adapter_fabric {

const char* to_string(ErrorCode code) noexcept {
  switch (code) {
    case ErrorCode::ok: return "ok";
    case ErrorCode::invalid_identity: return "invalid_identity";
    case ErrorCode::invalid_generation: return "invalid_generation";
    case ErrorCode::invalid_state_transition: return "invalid_state_transition";
    case ErrorCode::incompatibility: return "incompatibility";
    case ErrorCode::invalid_composition: return "invalid_composition";
    case ErrorCode::capacity_overcommit: return "capacity_overcommit";
    case ErrorCode::capacity_underflow: return "capacity_underflow";
    case ErrorCode::double_reservation: return "double_reservation";
    case ErrorCode::double_release: return "double_release";
    case ErrorCode::not_found: return "not_found";
    case ErrorCode::already_exists: return "already_exists";
    case ErrorCode::checksum_mismatch: return "checksum_mismatch";
    case ErrorCode::malformed: return "malformed";
    case ErrorCode::truncation: return "truncation";
    case ErrorCode::duplicate: return "duplicate";
    case ErrorCode::invalid_enum: return "invalid_enum";
    case ErrorCode::invalid_generation_value: return "invalid_generation_value";
    case ErrorCode::integer_overflow: return "integer_overflow";
    case ErrorCode::trailing_garbage: return "trailing_garbage";
    case ErrorCode::unsupported_version: return "unsupported_version";
    case ErrorCode::corrupt: return "corrupt";
    case ErrorCode::stale_authority: return "stale_authority";
    case ErrorCode::stale_worker: return "stale_worker";
    case ErrorCode::stale_boot: return "stale_boot";
    case ErrorCode::stale_attempt: return "stale_attempt";
    case ErrorCode::stale_generation: return "stale_generation";
    case ErrorCode::invalid_activation: return "invalid_activation";
    case ErrorCode::eviction_denied: return "eviction_denied";
    case ErrorCode::invalid_migration: return "invalid_migration";
    case ErrorCode::invalid_invalidation: return "invalid_invalidation";
    case ErrorCode::io_error: return "io_error";
    case ErrorCode::protocol_error: return "protocol_error";
    case ErrorCode::transport_error: return "transport_error";
    case ErrorCode::resource_unavailable: return "resource_unavailable";
    case ErrorCode::not_available: return "not_available";
    case ErrorCode::invalid_argument: return "invalid_argument";
    case ErrorCode::precondition_error: return "precondition_error";
    case ErrorCode::authority_conflict: return "authority_conflict";
    case ErrorCode::internal_error: return "internal_error";
  }
  return "unknown";
}

Error::Error(ErrorCode code, std::string message, std::string context)
    : std::runtime_error(std::move(message)), code_(code), context_(std::move(context)) {}

}  // namespace adapter_fabric
