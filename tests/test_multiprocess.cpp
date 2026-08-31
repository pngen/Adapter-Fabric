// Adapter Fabric — real multiprocess authority proof.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "adapter_fabric/proof.hpp"
#include <iostream>

AF_TEST_CASE(multiprocess_real_os_proof) {
  int r = adapter_fabric::run_multiprocess_proof(AF_COORDINATOR_EXE, AF_WORKER_EXE, 23001);
  AF_CHECK_EQ(r, 0);
  std::cout << "  (coordinator exe: " << AF_COORDINATOR_EXE << ")\n";
}

AF_TEST_MAIN
