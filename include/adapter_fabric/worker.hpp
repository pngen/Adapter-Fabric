// Adapter Fabric — worker service.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "adapter_fabric/identity.hpp"
#include "adapter_fabric/protocol.hpp"
#include "adapter_fabric/transport.hpp"

namespace adapter_fabric {

struct WorkerOptions {
  std::string coordinator_host = "127.0.0.1";
  std::uint16_t coordinator_port = 0;
  WorkerId id;
  NodeId node;
  DeviceId device;
  std::string name;
  bool use_cuda = false;
  std::uint64_t boot_seed = 1;   // deterministic boot id derivation
};

// A worker process-local service. Holds EPHEMERAL residency that is never
// resurrected after death: a fresh process gets a fresh WorkerBootId and a
// clean local state map.
class WorkerService {
 public:
  explicit WorkerService(WorkerOptions opts);
  ~WorkerService();

  bool connect_and_register();
  void run();          // blocking loop until coordinator disconnect / stop
  void stop() noexcept { stop_ = true; }

  WorkerBootId boot() const noexcept { return boot_; }
  bool registered() const noexcept { return registered_; }

  // ephemeral per-process local residency: adapter id -> (ready, bytes).
  std::map<std::string, std::pair<bool, std::uint64_t>> local_residency_;

 private:
  void handle_command(const Message& msg);
  void reply(const Message& req, MsgType type, bool ok, std::uint64_t bytes = 0, const std::string& note = {});

  WorkerOptions opts_;
  WorkerBootId boot_;
  TcpSocket sock_;
  bool registered_ = false;
  bool stop_ = false;
};

}  // namespace adapter_fabric