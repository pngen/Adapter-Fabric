// Adapter Fabric — CUDA host wrapper.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/cuda.hpp"
#include "cuda/detail.hpp"

namespace adapter_fabric {
namespace cuda {

#ifndef ADAPTER_FABRIC_HAS_CUDA
namespace detail {
bool is_available() { return false; }
const char* reason() noexcept { return "CUDA runtime not linked"; }
adapter_fabric::cuda::DeviceInfo info(int) { return {}; }
void* alloc(std::size_t) { return nullptr; }
void free_mem(void*) noexcept {}
bool memcpy_h2d(void*, const void*, std::size_t) { return false; }
bool memcpy_d2h(void*, const void*, std::size_t) { return false; }
bool lora_run(const float*, const float*, const float*, float*, int, int, int, float) { return false; }
}  // namespace detail
#endif

bool available() { return detail::is_available(); }
const char* availability_reason() noexcept { return detail::reason(); }
DeviceInfo device_info(int d) { return detail::info(d); }

DeviceBuffer allocate(std::size_t bytes) { return DeviceBuffer(detail::alloc(bytes), bytes); }
void release(DeviceBuffer& buf) noexcept { detail::free_mem(buf.ptr()); buf = DeviceBuffer{}; }

bool upload(const DeviceBuffer& buf, const void* host, std::size_t bytes) {
  if (!buf.valid() || bytes > buf.bytes()) return false;
  return detail::memcpy_h2d(buf.ptr(), host, bytes);
}
bool download(const DeviceBuffer& buf, void* host, std::size_t bytes) {
  if (!buf.valid() || bytes > buf.bytes()) return false;
  return detail::memcpy_d2h(host, buf.ptr(), bytes);
}

bool apply_lora(const DeviceBuffer& input, const DeviceBuffer& wup, const DeviceBuffer& wdown,
                DeviceBuffer& output, int M, int N, int R, float scale) {
  if (!input.valid() || !wup.valid() || !wdown.valid() || !output.valid()) return false;
  const std::size_t need = static_cast<std::size_t>(M) * static_cast<std::size_t>(N) * sizeof(float);
  if (output.bytes() < need || input.bytes() < need) return false;
  return detail::lora_run(static_cast<const float*>(input.ptr()),
                          static_cast<const float*>(wup.ptr()),
                          static_cast<const float*>(wdown.ptr()),
                          static_cast<float*>(output.ptr()), M, N, R, scale);
}

}  // namespace cuda
}  // namespace adapter_fabric
