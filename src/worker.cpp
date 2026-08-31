// Adapter Fabric — worker service implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/worker.hpp"
#include "adapter_fabric/rng.hpp"

namespace adapter_fabric {

WorkerService::WorkerService(WorkerOptions opts) : opts_(std::move(opts)) {}

WorkerService::~WorkerService() { sock_.close(); }

bool WorkerService::connect_and_register() {
  Rng boot_rng(opts_.boot_seed);
  boot_ = WorkerBootId::generate(boot_rng);
  if (!sock_.connect(opts_.coordinator_host, opts_.coordinator_port)) return false;
  Message reg; reg.type = MsgType::wk_register;
  reg.text = opts_.id.str();
  reg.text2 = boot_.str();
  reg.text3 = opts_.name + "@" + opts_.coordinator_host + ":" + std::to_string(opts_.coordinator_port);
  reg.text4 = opts_.node.str();
  if (!sock_.send_frame(encode_message(reg))) return false;
  std::vector<std::uint8_t> buf;
  if (!sock_.recv_frame(buf)) return false;
  Message ack = decode_message(buf);
  return ack.ok;
}

void WorkerService::reply(const Message& req, MsgType type, bool ok, std::uint64_t bytes, const std::string& note) {
  Message m; m.type = type; m.req_id = req.req_id; m.ok = ok; m.a = bytes; m.text = note;
  sock_.send_frame(encode_message(m));
}

void WorkerService::handle_command(const Message& msg) {
  switch (msg.type) {
    case MsgType::wk_load: {
      std::uint64_t bytes = msg.a;
      if (bytes == 0) bytes = 1;
      // ephemeral acceptance; a real backend would allocate here.
      local_residency_[msg.text] = {true, bytes};
      reply(msg, MsgType::wk_load_ack, true, bytes, "loaded");
      break;
    }
    case MsgType::wk_activate: {
      reply(msg, MsgType::wk_activate_ack, true, local_residency_.count(msg.text) ? local_residency_[msg.text].second : 0, "activated");
      break;
    }
    case MsgType::wk_deactivate: {
      reply(msg, MsgType::wk_deactivate_ack, true, 0, "deactivated");
      break;
    }
    case MsgType::wk_migrate: {
      reply(msg, MsgType::wk_migrate_ack, true, 0, "migrated");
      break;
    }
    case MsgType::wk_invalidate: {
      local_residency_.erase(msg.text);
      reply(msg, MsgType::wk_invalidate_ack, true, 0, "invalidated");
      break;
    }
    case MsgType::wk_shutdown: { stop_ = true; break; }
    default: break;
  }
}

void WorkerService::run() {
  std::vector<std::uint8_t> buf;
  while (!stop_ && sock_.recv_frame(buf)) {
    Message msg;
    try { msg = decode_message(buf); } catch (...) { break; }
    handle_command(msg);
  }
}

}  // namespace adapter_fabric