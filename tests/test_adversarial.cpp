// Adapter Fabric — adversarial malformed-input tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "adapter_fabric/protocol.hpp"
#include "adapter_fabric/persistence.hpp"
#include <random>

using namespace adapter_fabric;

AF_TEST_CASE(adversarial_decode_message_random_bytes) {
  std::mt19937_64 rng(1234);
  for (int i = 0; i < 200; ++i) {
    std::size_t n = rng() % 64;
    std::vector<std::uint8_t> bytes(n);
    for (auto& b : bytes) b = static_cast<std::uint8_t>(rng());
    bool threw = false;
    try { decode_message(bytes); } catch (...) { threw = true; }
    // either decoded, or threw; but a randomly truncated message should throw.
    if (n < 40) AF_CHECK(threw);
  }
}

AF_TEST_CASE(adversarial_decode_message_truncated_valid) {
  Message m; m.type = MsgType::ctl_list; m.text = "abc"; m.a = 42;
  auto bytes = encode_message(m);
  // remove the trailing 8 checksum bytes + 1 payload byte
  std::vector<std::uint8_t> cut(bytes.begin(), bytes.begin() + (static_cast<long>(bytes.size()) - 9));
  AF_CHECK_THROWS(decode_message(cut));
}

AF_TEST_CASE(adversarial_deserialize_snapshot_random_bytes) {
  std::mt19937_64 rng(99);
  for (int i = 0; i < 200; ++i) {
    std::size_t n = rng() % 80;
    std::vector<std::uint8_t> bytes(n);
    for (auto& b : bytes) b = static_cast<std::uint8_t>(rng());
    bool threw = false;
    try { deserialize_snapshot(bytes); } catch (...) { threw = true; }
    if (n < 40) AF_CHECK(threw);
  }
}

AF_TEST_CASE(adversarial_deserialize_adapter_random_bytes) {
  std::mt19937_64 rng(55);
  for (int i = 0; i < 200; ++i) {
    std::size_t n = rng() % 100;
    std::vector<std::uint8_t> bytes(n);
    for (auto& b : bytes) b = static_cast<std::uint8_t>(rng());
    bool threw = false;
    try { deserialize_adapter(bytes); } catch (...) { threw = true; }
  }
}

AF_TEST_MAIN
