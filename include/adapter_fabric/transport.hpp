// Adapter Fabric — framed TCP transport (Winsock).
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace adapter_fabric {

// One-shot Winsock initializer (idempotent).
void tcp_global_init();
void tcp_global_cleanup();

// A connected TCP stream endpoint bound to the framed wire format:
//   [4-byte length][payload][8-byte FNV-1a checksum of payload]
// Writer serialization is safe under concurrency via an internal mutex.
class TcpSocket {
 public:
  TcpSocket() = default;
  ~TcpSocket();
  TcpSocket(TcpSocket&& other) noexcept;
  TcpSocket& operator=(TcpSocket&& other) noexcept;
  TcpSocket(const TcpSocket&) = delete;
  TcpSocket& operator=(const TcpSocket&) = delete;

  bool connect(const std::string& host, std::uint16_t port);
  bool send_frame(const std::vector<std::uint8_t>& payload);
  bool recv_frame(std::vector<std::uint8_t>& payload);
  void close() noexcept;
  bool valid() const noexcept;
  std::string peer_address() const;

  // Put the socket into listening mode and return a connection on accept.
  bool bind_listen(std::uint16_t port);
  TcpSocket accept();

 private:
  bool send_all(const std::uint8_t* p, std::size_t n);
  bool recv_exact(std::uint8_t* p, std::size_t n);

  void* socket_ = nullptr;   // SOCKET (opaque to avoid windows.h in this header)
  std::mutex write_lock_;
};

}  // namespace adapter_fabric
