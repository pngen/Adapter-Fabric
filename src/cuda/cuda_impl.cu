// Adapter Fabric — CUDA backend implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "cuda/detail.hpp"
#include <cuda_runtime.h>

namespace adapter_fabric {
namespace cuda {
namespace detail {

bool is_available() { int c = 0; return cudaGetDeviceCount(&c) == cudaSuccess && c > 0; }
const char* reason() noexcept { return is_available() ? "CUDA available" : "no CUDA device"; }

DeviceInfo info(int device) {
  DeviceInfo d;
  int count = 0;
  cudaGetDeviceCount(&count);
  d.count = count;
  if (count <= 0 || device < 0 || device >= count) return d;
  cudaDeviceProp prop{};
  if (cudaGetDeviceProperties(&prop, device) != cudaSuccess) return d;
  d.available = true;
  d.name = prop.name;
  d.cc_major = prop.major;
  d.cc_minor = prop.minor;
  d.total_memory = static_cast<std::size_t>(prop.totalGlobalMem);
  return d;
}

void* alloc(std::size_t bytes) { void* p = nullptr; if (cudaMalloc(&p, bytes) != cudaSuccess) return nullptr; return p; }
void free_mem(void* p) noexcept { if (p) cudaFree(p); }
bool memcpy_h2d(void* dev, const void* host, std::size_t bytes) {
  if (cudaMemcpy(dev, host, bytes, cudaMemcpyHostToDevice) != cudaSuccess) return false;
  return cudaDeviceSynchronize() == cudaSuccess;
}
bool memcpy_d2h(void* host, const void* dev, std::size_t bytes) {
  if (cudaMemcpy(host, dev, bytes, cudaMemcpyDeviceToHost) != cudaSuccess) return false;
  return cudaDeviceSynchronize() == cudaSuccess;
}

__global__ void lora_kernel(const float* __restrict__ in, float* __restrict__ out,
                            const float* __restrict__ wup, const float* __restrict__ wdown,
                            int N, int R, float scale) {
  const int row = blockIdx.x;
  const int j = blockIdx.y * blockDim.x + threadIdx.x;
  if (j < N) {
    float acc = 0.0f;
    for (int r = 0; r < R; ++r) {
      acc += wdown[row * R + r] * wup[r * N + j];
    }
    out[row * N + j] = in[row * N + j] + scale * acc;
  }
}

bool lora_run(const float* in, const float* wup, const float* wdown, float* out,
              int M, int N, int R, float scale) {
  if (M <= 0 || N <= 0 || R <= 0) return false;
  dim3 block(256);
  dim3 grid(static_cast<unsigned int>(M), static_cast<unsigned int>((N + 255) / 256));
  lora_kernel<<<grid, block>>>(in, out, wup, wdown, N, R, scale);
  return cudaDeviceSynchronize() == cudaSuccess;
}

}  // namespace detail
}  // namespace cuda
}  // namespace adapter_fabric
