// Adapter Fabric — capacity accounting tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "adapter_fabric/capacity.hpp"

using namespace adapter_fabric;

AF_TEST_CASE(capacity_reserve_release_clean) {
  std::map<ResidencyLocation, std::uint64_t> lim{{ResidencyLocation::device_memory, 1024}};
  CapacityAccountant cap(lim);
  AF_CHECK(cap.is_clean());
  cap.reserve(ResidencyLocation::device_memory, 512, "a");
  AF_CHECK(!cap.is_clean());
  AF_CHECK_EQ(cap.used(ResidencyLocation::device_memory), 512);
  AF_CHECK_EQ(cap.free(ResidencyLocation::device_memory), 512);
  cap.release(ResidencyLocation::device_memory, 512, "a");
  AF_CHECK(cap.is_clean());
  AF_CHECK_EQ(cap.used(ResidencyLocation::device_memory), 0);
}

AF_TEST_CASE(capacity_overcommit_rejected) {
  std::map<ResidencyLocation, std::uint64_t> lim{{ResidencyLocation::device_memory, 100}};
  CapacityAccountant cap(lim);
  cap.reserve(ResidencyLocation::device_memory, 100, "a");
  AF_CHECK_THROWS(cap.reserve(ResidencyLocation::device_memory, 1, "b"));
  AF_CHECK(cap.over_limit(ResidencyLocation::device_memory) == false);
}

AF_TEST_CASE(capacity_double_reservation_rejected) {
  std::map<ResidencyLocation, std::uint64_t> lim{{ResidencyLocation::device_memory, 200}};
  CapacityAccountant cap(lim);
  cap.reserve(ResidencyLocation::device_memory, 100, "a");
  AF_CHECK_THROWS(cap.reserve(ResidencyLocation::device_memory, 100, "a"));
}

AF_TEST_CASE(capacity_double_release_rejected) {
  std::map<ResidencyLocation, std::uint64_t> lim{{ResidencyLocation::device_memory, 200}};
  CapacityAccountant cap(lim);
  cap.reserve(ResidencyLocation::device_memory, 100, "a");
  cap.release(ResidencyLocation::device_memory, 100, "a");
  AF_CHECK_THROWS(cap.release(ResidencyLocation::device_memory, 100, "a"));
}

AF_TEST_CASE(capacity_underflow_rejected) {
  std::map<ResidencyLocation, std::uint64_t> lim{{ResidencyLocation::device_memory, 200}};
  CapacityAccountant cap(lim);
  cap.reserve(ResidencyLocation::device_memory, 50, "a");
  AF_CHECK_THROWS(cap.release(ResidencyLocation::device_memory, 100, "a"));
}

AF_TEST_MAIN
