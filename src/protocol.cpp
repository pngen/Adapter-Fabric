// Adapter Fabric — protocol codec.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/protocol.hpp"
#include "adapter_fabric/error.hpp"
#include "adapter_fabric/persistence.hpp"
#include <cstring>

namespace adapter_fabric {

const char* to_string(MsgType t) noexcept {
  switch (t) {
    case MsgType::ctl_register_worker: return "ctl_register_worker";
    case MsgType::ctl_register_adapter: return "ctl_register_adapter";
    case MsgType::ctl_validate: return "ctl_validate";
    case MsgType::ctl_load: return "ctl_load";
    case MsgType::ctl_activate: return "ctl_activate";
    case MsgType::ctl_deactivate: return "ctl_deactivate";
    case MsgType::ctl_compose: return "ctl_compose";
    case MsgType::ctl_migrate: return "ctl_migrate";
    case MsgType::ctl_invalidate: return "ctl_invalidate";
    case MsgType::ctl_evict: return "ctl_evict";
    case MsgType::ctl_pin: return "ctl_pin";
    case MsgType::ctl_unpin: return "ctl_unpin";
    case MsgType::ctl_snapshot: return "ctl_snapshot";
    case MsgType::ctl_epoch: return "ctl_epoch";
    case MsgType::ctl_explain: return "ctl_explain";
    case MsgType::ctl_list: return "ctl_list";
    case MsgType::ctl_shutdown: return "ctl_shutdown";
    case MsgType::ctl_capacity: return "ctl_capacity";
    case MsgType::ctl_complete: return "ctl_complete";
    case MsgType::ctl_result: return "ctl_result";
    case MsgType::wk_register: return "wk_register";
    case MsgType::wk_register_ack: return "wk_register_ack";
    case MsgType::wk_inventory: return "wk_inventory";
    case MsgType::wk_ready: return "wk_ready";
    case MsgType::wk_complete: return "wk_complete";
    case MsgType::wk_error: return "wk_error";
    case MsgType::wk_load: return "wk_load";
    case MsgType::wk_load_ack: return "wk_load_ack";
    case MsgType::wk_activate: return "wk_activate";
    case MsgType::wk_activate_ack: return "wk_activate_ack";
    case MsgType::wk_deactivate: return "wk_deactivate";
    case MsgType::wk_deactivate_ack: return "wk_deactivate_ack";
    case MsgType::wk_migrate: return "wk_migrate";
    case MsgType::wk_migrate_ack: return "wk_migrate_ack";
    case MsgType::wk_invalidate: return "wk_invalidate";
    case MsgType::wk_invalidate_ack: return "wk_invalidate_ack";
    case MsgType::wk_shutdown: return "wk_shutdown";
    case MsgType::ping: return "ping";
    case MsgType::pong: return "pong";
  }
  return "unknown";
}

std::vector<std::uint8_t> encode_fence(const AuthorityFence& f) {
  BinWriter w;
  auto wu = [&](const Uuid& u){ const auto& b = u.bytes(); w.bytes(b.data(), b.size()); };
  wu(f.adapter.uuid()); wu(f.composition.uuid());
  write_gen(w, f.adapter_generation); write_gen(w, f.composition_generation);
  write_gen(w, f.residency_generation); write_gen(w, f.artifact_generation);
  wu(f.base_model_revision.uuid());
  write_gen(w, f.epoch);
  wu(f.worker.uuid()); wu(f.boot.uuid()); wu(f.attempt.uuid()); wu(f.device.uuid()); wu(f.node.uuid());
  return w.finish();
}

AuthorityFence decode_fence(const std::string& bytes) {
  std::span<const std::uint8_t> sp(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
  return decode_fence(sp);
}

AuthorityFence decode_fence(std::span<const std::uint8_t> bytes) {
  BinReader r(bytes);
  auto ru = [&](){ auto v = r.bytes(16); std::array<std::uint8_t, 16> a{}; for (std::size_t i = 0; i < 16; ++i) a[i] = v[i]; return Uuid{a}; };
  AuthorityFence f;
  f.adapter = AdapterId{ru()}; f.composition = CompositionId{ru()};
  f.adapter_generation = read_gen<gen::adapter>(r); f.composition_generation = read_gen<gen::composition>(r);
  f.residency_generation = read_gen<gen::residency>(r); f.artifact_generation = read_gen<gen::artifact>(r);
  f.base_model_revision = ModelRevisionId{ru()};
  f.epoch = read_gen<gen::coordinator_epoch>(r);
  f.worker = WorkerId{ru()}; f.boot = WorkerBootId{ru()}; f.attempt = AttemptId{ru()}; f.device = DeviceId{ru()}; f.node = NodeId{ru()};
  r.expect_end(r.size() - 8);
  return f;
}

std::vector<std::uint8_t> encode_message(const Message& m) {
  BinWriter w;
  w.u8(static_cast<std::uint8_t>(m.type));
  w.string(m.text);
  w.string(m.text2);
  w.string(m.text3);
  w.string(m.text4);
  w.u64(m.a);
  w.u64(m.b);
  w.u64(m.c);
  w.u64(m.req_id);
  w.u8(m.ok ? 1 : 0);
  w.u32(static_cast<std::uint32_t>(m.payload.size()));
  w.bytes(m.payload.data(), m.payload.size());
  return w.finish();
}

Message decode_message(std::span<const std::uint8_t> bytes) {
  BinReader r(bytes);
  Message m;
  m.type = static_cast<MsgType>(r.u8());
  m.text = r.string();
  m.text2 = r.string();
  m.text3 = r.string();
  m.text4 = r.string();
  m.a = r.u64();
  m.b = r.u64();
  m.c = r.u64();
  m.req_id = r.u64();
  m.ok = r.u8() != 0;
  std::uint32_t n = r.u32();
  if (n > r.size()) throw Error(ErrorCode::malformed, "payload length exceeds message");
  m.payload = r.bytes(n);
  r.expect_end(r.size() - 8);
  return m;
}

}  // namespace adapter_fabric