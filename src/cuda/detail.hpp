// Adapter Fabric — internal CUDA detail interface (not installed).
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "adapter_fabric/cuda.hpp"

namespace adapter_fabric {
namespace cuda {
namespace detail {

bool is_available();
const char* reason() noexcept;
adapter_fabric::cuda::DeviceInfo info(int device);
void* alloc(std::size_t bytes);
void free_mem(void* p) noexcept;
bool memcpy_h2d(void* dev, const void* host, std::size_t bytes);
bool memcpy_d2h(void* host, const void* dev, std::size_t bytes);
bool lora_run(const float* in, const float* wup, const float* wdown, float* out,
              int M, int N, int R, float scale);

}  // namespace detail
}  // namespace cuda
}  // namespace adapter_fabric
