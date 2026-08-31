// Adapter Fabric — real RTX 5090 CUDA proof.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "adapter_fabric/cuda.hpp"
#include "adapter_fabric/proof.hpp"
#include <iostream>

AF_TEST_CASE(cuda_real_proof) {
  if (!adapter_fabric::cuda::available()) {
    std::cout << "  (CUDA not available; proof skipped)\n";
    std::cerr << "  SKIP: " << adapter_fabric::cuda::availability_reason() << "\n";
    return;
  }
  int r = adapter_fabric::run_cuda_proof();
  AF_CHECK_EQ(r, 0);
}

AF_TEST_MAIN
