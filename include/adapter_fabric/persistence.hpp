// Adapter Fabric — strict versioned persistence.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include "adapter_fabric/adapter.hpp"
#include "adapter_fabric/composition.hpp"
#include "adapter_fabric/generation.hpp"
#include "adapter_fabric/identity.hpp"
#include "adapter_fabric/inventory.hpp"

namespace adapter_fabric {

// A deterministic little-endian binary writer.
class BinWriter {
 public:
  void u8(std::uint8_t v);
  void u16(std::uint16_t v);
  void u32(std::uint32_t v);
  void u64(std::uint64_t v);
  void i64(std::int64_t v);
  void f32(float v);
  void f64(double v);
  void string(std::string_view s);
  void bytes(const std::uint8_t* p, std::size_t n);
  void bytes(std::string_view s);

  const std::vector<std::uint8_t>& data() const noexcept { return buf_; }
  std::vector<std::uint8_t> take() { return std::move(buf_); }

  // Append the FNV-1a checksum of the current payload as the final 8 bytes.
  std::vector<std::uint8_t> finish();

 private:
  std::vector<std::uint8_t> buf_;
};

// A strict binary reader that validates bounds on every read and the whole
// payload checksum up front. Any malformed input throws deterministically.
class BinReader {
 public:
  BinReader(std::span<const std::uint8_t> data);

  std::uint8_t u8();
  std::uint16_t u16();
  std::uint32_t u32();
  std::uint64_t u64();
  std::int64_t i64();
  float f32();
  double f64();
  std::string string();
  std::vector<std::uint8_t> bytes(std::size_t n);

  std::size_t offset() const noexcept { return pos_; }
  std::size_t size() const noexcept { return data_.size(); }
  bool eof() const noexcept { return pos_ == data_.size(); }

  // Reject any trailing bytes after the expected end marker.
  void expect_end(std::size_t expected_end_offset);

 private:
  void check(std::size_t n);
  std::span<const std::uint8_t> data_;
  std::size_t pos_ = 0;
  std::uint64_t checksum_ = 0;
  std::size_t payload_end_ = 0;
};

// Primitive identity/generation codecs.
template <typename Tag>
void write_id(BinWriter& w, const Id<Tag>& id) {
  const auto& b = id.uuid().bytes();
  w.bytes(b.data(), b.size());
}

template <typename Tag>
Id<Tag> read_id(BinReader& r) {
  auto b = r.bytes(16);
  std::array<std::uint8_t, 16> a{};
  for (std::size_t i = 0; i < 16; ++i) a[i] = b[i];
  return Id<Tag>{Uuid{a}};
}

template <typename Tag>
void write_gen(BinWriter& w, const Generation<Tag>& g) { w.u64(g.value()); }

template <typename Tag>
Generation<Tag> read_gen(BinReader& r) { return Generation<Tag>{r.u64()}; }

// A snapshot of the durable canonical state that must survive restart. It
// intentionally does NOT include any process-local residency or activation.
struct FabricSnapshot {
  static constexpr std::uint32_t kFormat = 1;
  CoordinatorEpoch epoch;
  AdapterGeneration next_adapter_generation;
  CompositionGeneration next_composition_generation;
  std::vector<AdapterDescriptor> adapters;
  std::vector<Composition> compositions;
  std::vector<WorkerRecord> workers;
};

// Strict codecs. deserialize validates the version, checksum, every field,
// and rejects trailing garbage, duplicate ids, invalid enums/generations and
// inconsistent composition membership.
std::vector<std::uint8_t> serialize_adapter(const AdapterDescriptor& d);
AdapterDescriptor deserialize_adapter(std::span<const std::uint8_t> bytes);
AdapterDescriptor deserialize_adapter(const std::string& bytes);

std::vector<std::uint8_t> serialize_snapshot(const FabricSnapshot& s);
FabricSnapshot deserialize_snapshot(std::span<const std::uint8_t> bytes);

}  // namespace adapter_fabric