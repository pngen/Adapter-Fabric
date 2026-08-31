// Adapter Fabric — coordinator service.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/protocol.hpp"
#include "adapter_fabric/transport.hpp"

namespace adapter_fabric {

// A coordinator hosting the canonical Fabric and bridging control clients
// to registered workers over the framed TCP protocol. Network I/O is never
// performed while holding the global fabric state lock.
class CoordinatorServer {
 public:
  explicit CoordinatorServer(std::uint16_t port, std::map<ResidencyLocation, std::uint64_t> limits = {});
  ~CoordinatorServer();

  bool start();
  void stop();
  std::uint16_t port() const noexcept { return port_; }
  bool running() const noexcept { return running_; }
  Fabric& fabric() noexcept { return fabric_; }

  // Synchronous call used by the control path: process one control message.
  // Returns the response message to send back to the caller.
  Message process_control(const Message& req);

 private:
  void accept_loop();
  void handle_connection(std::shared_ptr<TcpSocket> conn);
  Message forward_to_worker(std::uint64_t req_id, const std::string& worker_id, const Message& fwd);
  void abort_pending_for_worker(const std::string& worker_id);

  struct WorkerSlot {
    std::shared_ptr<TcpSocket> sock;
    WorkerId id;
    WorkerBootId boot;
    std::string address;
    bool connected = false;
  };

  struct Pending {
    std::mutex m;
    std::condition_variable cv;
    bool done = false;
    Message result;
  };

  std::uint16_t port_;
  std::atomic<bool> running_{false};
  std::thread accept_thread_;
  std::vector<std::thread> conn_threads_;

  Fabric fabric_;
  std::map<std::string, std::shared_ptr<WorkerSlot>> workers_;
  std::mutex workers_mc_;
  std::mutex state_mc_;              // guards state mutation; no net I/O under it
  std::atomic<std::uint64_t> next_req_{1};
  std::map<std::uint64_t, std::shared_ptr<Pending>> pending_;
  std::mutex pending_mc_;
  std::atomic<bool> shutdown_{false};
  TcpSocket accept_listener_;
  uint64_t wsa_placeholder_ = 0;
};

}  // namespace adapter_fabric