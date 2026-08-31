// Adapter Fabric — framed wire protocol.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include "adapter_fabric/authority.hpp"

namespace adapter_fabric {

// Operation codes for the coordinator<->worker and client<->coordinator path.
enum class MsgType : std::uint8_t {
  // client -> coordinator (control).
  ctl_register_worker = 0x10,
  ctl_register_adapter = 0x11,
  ctl_validate = 0x12,
  ctl_load = 0x13,
  ctl_activate = 0x14,
  ctl_deactivate = 0x15,
  ctl_compose = 0x16,
  ctl_migrate = 0x17,
  ctl_invalidate = 0x18,
  ctl_evict = 0x19,
  ctl_pin = 0x1A,
  ctl_unpin = 0x1B,
  ctl_snapshot = 0x1C,
  ctl_explain = 0x1D,
  ctl_list = 0x1E,
  ctl_shutdown = 0x2E,
  ctl_epoch = 0x1F,
  ctl_capacity = 0x20,
  ctl_complete = 0x21,
  ctl_result = 0x2F,
  // worker -> coordinator.
  wk_register = 0x30,
  wk_register_ack = 0x31,
  wk_inventory = 0x32,
  wk_ready = 0x33,
  wk_complete = 0x34,
  wk_error = 0x35,
  // coordinator -> worker.
  wk_load = 0x40,
  wk_load_ack = 0x41,
  wk_activate = 0x42,
  wk_activate_ack = 0x43,
  wk_deactivate = 0x44,
  wk_deactivate_ack = 0x45,
  wk_migrate = 0x46,
  wk_migrate_ack = 0x47,
  wk_invalidate = 0x48,
  wk_invalidate_ack = 0x49,
  wk_shutdown = 0x4F,
  // general.
  ping = 0x50,
  pong = 0x51,
};

const char* to_string(MsgType t) noexcept;

// A generic message carrying identities (as canonical strings), generations,
// one digest payload, and a free-form error/summary. The codec is strict.
struct Message {
  MsgType type = MsgType::ping;
  std::string text;       // primary identity (adapter id, worker id, ...)
  std::string text2;      // secondary identity (worker id, instance id, ...)
  std::string text3;      // tertiary (composition id, base model revision, ...)
  std::string text4;      // free-form (reason / summary)
  std::uint64_t a = 0;    // generation / epoch / attempt value
  std::uint64_t b = 0;    // second number
  std::uint64_t c = 0;    // third number
  std::uint64_t req_id = 0; // correlation id for request/response bridging
  bool ok = false;        // generic success flag
  std::vector<std::uint8_t> payload;  // opaque bytes (adapter descriptor, snapshot, fence, ...)
};

// Strict deterministic codec. decode throws Error on malformed input.
std::vector<std::uint8_t> encode_fence(const AuthorityFence& f);
AuthorityFence decode_fence(std::span<const std::uint8_t> bytes);
AuthorityFence decode_fence(const std::string& bytes);

std::vector<std::uint8_t> encode_message(const Message& m);
Message decode_message(std::span<const std::uint8_t> bytes);

}  // namespace adapter_fabric