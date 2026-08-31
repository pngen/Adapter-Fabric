// Adapter Fabric — inventory of workers and adapter residency.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "adapter_fabric/generation.hpp"
#include "adapter_fabric/identity.hpp"

namespace adapter_fabric {

struct WorkerCapability {
  std::string name;
  std::string value;
};

// The durable record for a registered worker. Does not hold process-local
// adapter state; it records membership, capability, and the latest seen boot.
struct WorkerRecord {
  WorkerId id;
  WorkerBootId boot;
  std::string address;     // host:port of the worker transport
  NodeId node;
  DeviceId device;
  bool connected = false;
  CoordinatorEpoch epoch;
  std::vector<WorkerCapability> capabilities;
  std::uint64_t device_memory_bytes = 0;
  // adapter ids this worker reported as locally resident / ready (ephemeral).
  std::vector<AdapterId> local_adapters;
};

}  // namespace adapter_fabric
