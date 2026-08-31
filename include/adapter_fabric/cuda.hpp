// Adapter Fabric — CUDA accelerator backend (host API).
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstddef>
#include <string>

namespace adapter_fabric {
namespace cuda {

struct DeviceInfo {
  bool available = false;
  int count = 0;
  std::string name;
  int cc_major = 0;
  int cc_minor = 0;
  std::size_t total_memory = 0;
};

// True if a CUDA-capable device exists and the runtime was linked.
bool available();
const char* availability_reason() noexcept;
DeviceInfo device_info(int device = 0);

class DeviceBuffer {
 public:
  DeviceBuffer() = default;
  DeviceBuffer(void* p, std::size_t b) : ptr_(p), bytes_(b) {}
  void* ptr() const noexcept { return ptr_; }
  std::size_t bytes() const noexcept { return bytes_; }
  bool valid() const noexcept { return ptr_ != nullptr && bytes_ > 0; }
 private:
  void* ptr_ = nullptr;
  std::size_t bytes_ = 0;
};

DeviceBuffer allocate(std::size_t bytes);
void release(DeviceBuffer& buf) noexcept;
bool upload(const DeviceBuffer& buf, const void* host, std::size_t bytes);
bool download(const DeviceBuffer& buf, void* host, std::size_t bytes);

// y = x + scale * (W_down * W_up) @ x  for a batch of M rows, N columns and low
// rank R. All buffers are row-major float32. Verifiable against a CPU reference.
bool apply_lora(const DeviceBuffer& input, const DeviceBuffer& wup, const DeviceBuffer& wdown,
                DeviceBuffer& output, int M, int N, int R, float scale);

}  // namespace cuda
}  // namespace adapter_fabric
