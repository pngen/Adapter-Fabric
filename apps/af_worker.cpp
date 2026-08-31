// Adapter Fabric — worker executable.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/worker.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <variant>

using namespace adapter_fabric;

int main(int argc, char** argv) {
  WorkerOptions o;
  o.coordinator_host = "127.0.0.1";
  o.coordinator_port = 24000;
  o.name = "worker";
  o.boot_seed = 1;
  std::string id_str;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) o.coordinator_port = static_cast<std::uint16_t>(std::atoi(argv[++i]));
    else if (std::strcmp(argv[i], "--host") == 0 && i + 1 < argc) o.coordinator_host = argv[++i];
    else if (std::strcmp(argv[i], "--name") == 0 && i + 1 < argc) o.name = argv[++i];
    else if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc) o.boot_seed = std::strtoull(argv[++i], nullptr, 10);
    else if (std::strcmp(argv[i], "--id") == 0 && i + 1 < argc) id_str = argv[++i];
    else if (std::strcmp(argv[i], "--help") == 0) { std::cout << "usage: af_worker --port <port> --name <name> --seed <n> --id <uuid>\n"; return 0; }
  }
  if (!id_str.empty()) {
    auto v = WorkerId::parse(id_str);
    if (auto* id = std::get_if<WorkerId>(&v)) o.id = *id;
  } else {
    o.id = WorkerId::generate();
  }
  o.node = NodeId::generate();
  o.device = DeviceId::generate();
  WorkerService worker(std::move(o));
  if (!worker.connect_and_register()) { std::cerr << "worker " << worker.boot().str() << " failed to register\n"; return 1; }
  std::cout << "ADAPTER_FABRIC_WORKER id=" << worker.boot().str() << "\n";
  std::cout.flush();
  worker.run();
  return 0;
}

