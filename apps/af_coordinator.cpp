// Adapter Fabric — coordinator executable.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/coordinator.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

using namespace adapter_fabric;

int main(int argc, char** argv) {
  std::uint16_t port = 24000;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = static_cast<std::uint16_t>(std::atoi(argv[++i]));
    else if (std::strcmp(argv[i], "--help") == 0) { std::cout << "usage: af_coordinator --port <port>\n"; return 0; }
  }
  CoordinatorServer server(port);
  if (!server.start()) { std::cerr << "coordinator failed to start on port " << port << "\n"; return 1; }
  std::cout << "ADAPTER_FABRIC_COORDINATOR port=" << port << "\n";
  std::cout.flush();
  while (server.running()) std::this_thread::sleep_for(std::chrono::milliseconds(50));
  return 0;
}

