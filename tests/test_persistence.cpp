// Adapter Fabric — persistence round-trip and corruption rejection tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "fixtures.hpp"
#include "adapter_fabric/persistence.hpp"
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/checksum.hpp"

using namespace adapter_fabric;
namespace fx = af_test_fixture;

static std::vector<std::uint8_t> rechecksum(const std::vector<std::uint8_t>& in) {
  std::vector<std::uint8_t> out(in.begin(), in.end() - 8);
  std::uint64_t h = fnv1a_64(1469598103934665603ULL, out.data(), out.size());
  for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>((h >> (8 * i)) & 0xFF));
  return out;
}

AF_TEST_CASE(persistence_roundtrip) {
  Fabric f;
  auto bm = fx::base(); auto mr = fx::rev();
  f.register_adapter(fx::make_lora("a", bm, mr));
  AdapterDescriptor d2 = fx::make_lora("b", bm, mr); d2.targets[0].name = "v_proj";
  f.register_adapter(d2);
  WorkerRecord w; w.id = WorkerId::generate(); w.boot = WorkerBootId::generate(); w.connected = true; w.device = DeviceId::generate();
  f.register_worker(w);
  FabricSnapshot snap = f.snapshot();
  auto bytes = serialize_snapshot(snap);
  FabricSnapshot back = deserialize_snapshot(bytes);
  AF_CHECK_EQ(back.adapters.size(), snap.adapters.size());
  AF_CHECK_EQ(back.workers.size(), snap.workers.size());
}

AF_TEST_CASE(persistence_checksum_mismatch_rejected) {
  FabricSnapshot snap;
  snap.epoch = CoordinatorEpoch{2};
  snap.next_adapter_generation = AdapterGeneration{3};
  snap.next_composition_generation = CompositionGeneration{4};
  auto bytes = serialize_snapshot(snap);
  bytes[10] ^= 0xFF;   // corrupt a byte in the middle (checksum now wrong)
  AF_CHECK_THROWS(deserialize_snapshot(bytes));
}

AF_TEST_CASE(persistence_truncation_rejected) {
  FabricSnapshot snap;
  snap.epoch = CoordinatorEpoch{2};
  auto bytes = serialize_snapshot(snap);
  std::vector<std::uint8_t> cut(bytes.begin(), bytes.begin() + (static_cast<long>(bytes.size()) - 3));
  AF_CHECK_THROWS(deserialize_snapshot(cut));
}

AF_TEST_CASE(persistence_trailing_garbage_rejected) {
  FabricSnapshot snap;
  auto bytes = serialize_snapshot(snap);
  bytes.push_back(0xAB);
  bytes.push_back(0xCD);
  auto fixed = rechecksum(bytes);   // append garbage but keep checksum valid
  AF_CHECK_THROWS(deserialize_snapshot(fixed));
}

AF_TEST_CASE(persistence_unsupported_version_rejected) {
  FabricSnapshot snap;
  auto bytes = serialize_snapshot(snap);
  auto fixed = rechecksum(bytes);
  // first 4 bytes are the 32-bit format; set to an unsupported version.
  fixed[0] = 0xFF; fixed[1] = 0xFF; fixed[2] = 0xFF; fixed[3] = 0xFF;
  auto fixed2 = rechecksum(fixed);
  AF_CHECK_THROWS(deserialize_snapshot(fixed2));
}

AF_TEST_CASE(persistence_duplicate_id_rejected) {
  FabricSnapshot snap;
  auto bm = fx::base(); auto mr = fx::rev();
  auto d = fx::make_lora("a", bm, mr);
  snap.adapters.push_back(d);
  AdapterDescriptor dup = d;
  snap.adapters.push_back(dup);   // duplicate id
  auto bytes = serialize_snapshot(snap);
  AF_CHECK_THROWS(deserialize_snapshot(bytes));
}

AF_TEST_MAIN