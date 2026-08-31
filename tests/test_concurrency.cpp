// Adapter Fabric — concurrency tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "fixtures.hpp"
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/checksum.hpp"
#include "adapter_fabric/transport.hpp"
#include "adapter_fabric/protocol.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

using namespace adapter_fabric;
namespace fx = af_test_fixture;

AF_TEST_CASE(concurrency_fabric_operations_locked) {
  Fabric f;
  std::mutex m;
  std::atomic<std::int64_t> failures{0};
  auto bm = fx::base(); auto mr = fx::rev();
  std::vector<std::thread> threads;
  for (int t = 0; t < 8; ++t) {
    threads.emplace_back([&, t]() {
      try {
        for (int i = 0; i < 200; ++i) {
          auto d = fx::make_lora("adapter_" + std::to_string(t) + "_" + std::to_string(i), bm, mr);
          std::lock_guard<std::mutex> lk(m);
          auto reg = f.register_adapter(d);
          auto rep = f.validate(reg.id, fx::target(bm, mr));
          WorkerRecord w; w.id = WorkerId::generate(); w.boot = WorkerBootId::generate(); w.connected = true; w.device = DeviceId::generate();
          f.register_worker(w);
          auto dev = DeviceId::generate();
          f.create_instance(reg.id, w.id, w.boot, dev, reg.memory_bytes);
          AuthorityFence fence; fence.adapter = reg.id; fence.epoch = f.epoch();
          fence.adapter_generation = reg.generation; fence.base_model_revision = reg.base_model_revision;
          fence.worker = w.id; fence.boot = w.boot; fence.attempt = AttemptId::generate(); fence.device = dev;
          fence.residency_generation = ResidencyGeneration{1}; fence.composition_generation = CompositionGeneration{1}; fence.artifact_generation = ArtifactGeneration{1};
          auto chk = f.check_fence(fence);
          if (!chk.authorized()) ++failures;
          auto snap = f.snapshot(); (void)snap;
          f.capacity().reserve(ResidencyLocation::device_memory, reg.memory_bytes, "t" + std::to_string(t) + "_" + std::to_string(i));
          f.capacity().release(ResidencyLocation::device_memory, reg.memory_bytes, "t" + std::to_string(t) + "_" + std::to_string(i));
        }
      } catch (...) { ++failures; }
    });
  }
  for (auto& th : threads) th.join();
  AF_CHECK_EQ(failures.load(), 0);
  AF_CHECK(f.capacity().is_clean());
}

AF_TEST_CASE(concurrency_transport_frame_integrity) {
  std::uint16_t port = 22640;
  TcpSocket listener; listener.bind_listen(port);
  AF_CHECK(listener.valid());
  std::atomic<std::int64_t> mismatches{0};
  std::vector<bool> seen(200, false);
  std::thread server([&]() {
    TcpSocket conn = listener.accept();
    for (int i = 0; i < 200; ++i) {
      std::vector<std::uint8_t> buf;
      if (!conn.recv_frame(buf)) { ++mismatches; return; }
      Message m = decode_message(buf);
      if (m.a < 200) {
        if (seen[static_cast<std::size_t>(m.a)]) ++mismatches;  // duplicate
        seen[static_cast<std::size_t>(m.a)] = true;
      } else { ++mismatches; }
    }
  });
  TcpSocket client;
  AF_CHECK(client.connect("127.0.0.1", port));
  std::vector<std::thread> senders;
  for (int t = 0; t < 4; ++t) {
    senders.emplace_back([&, t]() {
      for (int i = 0; i < 50; ++i) {
        Message m; m.type = MsgType::ctl_list; m.text = "t" + std::to_string(t); m.a = static_cast<std::uint64_t>(t * 50 + i);
        auto payload = encode_message(m);
        if (!client.send_frame(payload)) ++mismatches;
      }
    });
  }
  for (auto& s : senders) s.join();
  server.join();
  AF_CHECK_EQ(mismatches.load(), 0);
}

AF_TEST_MAIN