// Adapter Fabric — framed TCP transport (Winsock) implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/transport.hpp"
#include "adapter_fabric/checksum.hpp"
#include "adapter_fabric/error.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <cstddef>

#pragma comment(lib, "ws2_32.lib")

namespace adapter_fabric {

namespace {
SOCKET raw(void* s) noexcept { return static_cast<SOCKET>(reinterpret_cast<std::uintptr_t>(s)); }
void* tovoid(SOCKET s) noexcept { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(s)); }
}  // namespace

void tcp_global_init() { static bool inited = false; if (!inited) { WSADATA d; WSAStartup(MAKEWORD(2, 2), &d); inited = true; } }
void tcp_global_cleanup() { WSACleanup(); }

TcpSocket::~TcpSocket() { close(); }

TcpSocket::TcpSocket(TcpSocket&& other) noexcept {
  socket_ = other.socket_;
  other.socket_ = nullptr;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
  if (this != &other) { close(); socket_ = other.socket_; other.socket_ = nullptr; }
  return *this;
}

bool TcpSocket::valid() const noexcept { return socket_ != nullptr; }

bool TcpSocket::connect(const std::string& host, std::uint16_t port) {
  tcp_global_init();
  SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s == INVALID_SOCKET) return false;
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
  if (::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    closesocket(s);
    return false;
  }
  socket_ = tovoid(s);
  return true;
}

bool TcpSocket::bind_listen(std::uint16_t port) {
  tcp_global_init();
  SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s == INVALID_SOCKET) return false;
  BOOL reuse = TRUE;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) { closesocket(s); return false; }
  if (::listen(s, SOMAXCONN) == SOCKET_ERROR) { closesocket(s); return false; }
  socket_ = tovoid(s);
  return true;
}

TcpSocket TcpSocket::accept() {
  TcpSocket out;
  sockaddr_in addr{};
  int len = sizeof(addr);
  SOCKET c = ::accept(raw(socket_), reinterpret_cast<sockaddr*>(&addr), &len);
  if (c != INVALID_SOCKET) out.socket_ = tovoid(c);
  return out;
}

bool TcpSocket::send_all(const std::uint8_t* p, std::size_t n) {
  while (n > 0) {
    int sent = ::send(raw(socket_), reinterpret_cast<const char*>(p), static_cast<int>(n), 0);
    if (sent == SOCKET_ERROR) return false;
    if (sent == 0) return false;
    p += sent;
    n -= static_cast<std::size_t>(sent);
  }
  return true;
}

bool TcpSocket::recv_exact(std::uint8_t* p, std::size_t n) {
  while (n > 0) {
    int got = ::recv(raw(socket_), reinterpret_cast<char*>(p), static_cast<int>(n), 0);
    if (got == SOCKET_ERROR) return false;
    if (got == 0) return false;
    p += got;
    n -= static_cast<std::size_t>(got);
  }
  return true;
}

bool TcpSocket::send_frame(const std::vector<std::uint8_t>& payload) {
  std::lock_guard<std::mutex> lk(write_lock_);
  if (!valid()) return false;
  std::uint32_t len = static_cast<std::uint32_t>(payload.size());
  std::uint8_t hdr[4];
  for (int i = 0; i < 4; ++i) hdr[i] = static_cast<std::uint8_t>((len >> (8 * i)) & 0xFF);
  std::uint64_t chk = fnv1a_64(1469598103934665603ULL, payload.data(), payload.size());
  if (!send_all(hdr, 4)) return false;
  if (!payload.empty() && !send_all(payload.data(), payload.size())) return false;
  std::uint8_t ck[8];
  for (int i = 0; i < 8; ++i) ck[i] = static_cast<std::uint8_t>((chk >> (8 * i)) & 0xFF);
  return send_all(ck, 8);
}

bool TcpSocket::recv_frame(std::vector<std::uint8_t>& payload) {
  if (!valid()) return false;
  std::uint8_t hdr[4];
  if (!recv_exact(hdr, 4)) return false;
  std::uint32_t len = 0;
  for (int i = 0; i < 4; ++i) len |= (static_cast<std::uint32_t>(hdr[i]) << (8 * i));
  if (len > (64u * 1024u * 1024u)) return false;  // hard cap
  payload.resize(len);
  if (len > 0 && !recv_exact(payload.data(), len)) return false;
  std::uint8_t ck[8];
  if (!recv_exact(ck, 8)) return false;
  std::uint64_t stored = 0;
  for (int i = 0; i < 8; ++i) stored |= (static_cast<std::uint64_t>(ck[i]) << (8 * i));
  std::uint64_t computed = fnv1a_64(1469598103934665603ULL, payload.data(), payload.size());
  return stored == computed;
}

void TcpSocket::close() noexcept {
  if (socket_ != nullptr) { closesocket(raw(socket_)); socket_ = nullptr; }
}

std::string TcpSocket::peer_address() const {
  if (!valid()) return "";
  sockaddr_in addr{};
  int len = sizeof(addr);
  getpeername(raw(socket_), reinterpret_cast<sockaddr*>(&addr), &len);
  char buf[INET_ADDRSTRLEN] = {0};
  inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
  return std::string(buf) + ":" + std::to_string(ntohs(addr.sin_port));
}

}  // namespace adapter_fabric