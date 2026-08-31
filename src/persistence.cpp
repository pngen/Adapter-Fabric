// Adapter Fabric — strict persistence implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/persistence.hpp"
#include "adapter_fabric/checksum.hpp"
#include "adapter_fabric/error.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>

namespace adapter_fabric {

// ---- BinWriter ----
void BinWriter::u8(std::uint8_t v) { buf_.push_back(v); }
void BinWriter::u16(std::uint16_t v) { buf_.push_back(static_cast<std::uint8_t>(v & 0xFF)); buf_.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF)); }
void BinWriter::u32(std::uint32_t v) { for (int i = 0; i < 4; ++i) buf_.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF)); }
void BinWriter::u64(std::uint64_t v) { for (int i = 0; i < 8; ++i) buf_.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF)); }
void BinWriter::i64(std::int64_t v) { u64(static_cast<std::uint64_t>(v)); }
void BinWriter::f32(float v) { std::uint32_t b; std::memcpy(&b, &v, 4); u32(b); }
void BinWriter::f64(double v) { std::uint64_t b; std::memcpy(&b, &v, 8); u64(b); }
void BinWriter::string(std::string_view s) { u32(static_cast<std::uint32_t>(s.size())); bytes(reinterpret_cast<const std::uint8_t*>(s.data()), s.size()); }
void BinWriter::bytes(const std::uint8_t* p, std::size_t n) { buf_.insert(buf_.end(), p, p + n); }
void BinWriter::bytes(std::string_view s) { bytes(reinterpret_cast<const std::uint8_t*>(s.data()), s.size()); }
std::vector<std::uint8_t> BinWriter::finish() { std::uint64_t h = fnv1a_64(1469598103934665603ULL, buf_.data(), buf_.size()); u64(h); return std::move(buf_); }

// ---- BinReader ----
BinReader::BinReader(std::span<const std::uint8_t> data) : data_(data) {
  if (data_.size() < 8) throw Error(ErrorCode::malformed, "payload too small");
  std::uint64_t stored = 0;
  for (int i = 0; i < 8; ++i) stored |= (static_cast<std::uint64_t>(data_[data_.size() - 8 + i]) << (8 * i));
  payload_end_ = data_.size() - 8;
  std::uint64_t computed = fnv1a_64(1469598103934665603ULL, data_.data(), payload_end_);
  if (computed != stored) throw Error(ErrorCode::checksum_mismatch, "checksum mismatch");
}

void BinReader::check(std::size_t n) {
  if (pos_ > payload_end_ || n > payload_end_ - pos_) throw Error(ErrorCode::truncation, "read past payload");
}

std::uint8_t BinReader::u8() { check(1); return data_[pos_++]; }
std::uint16_t BinReader::u16() { check(2); std::uint16_t v = static_cast<std::uint16_t>(data_[pos_]) | (static_cast<std::uint16_t>(data_[pos_ + 1]) << 8); pos_ += 2; return v; }
std::uint32_t BinReader::u32() { check(4); std::uint32_t v = 0; for (int i = 0; i < 4; ++i) v |= (static_cast<std::uint32_t>(data_[pos_ + i]) << (8 * i)); pos_ += 4; return v; }
std::uint64_t BinReader::u64() { check(8); std::uint64_t v = 0; for (int i = 0; i < 8; ++i) v |= (static_cast<std::uint64_t>(data_[pos_ + i]) << (8 * i)); pos_ += 8; return v; }
std::int64_t BinReader::i64() { return static_cast<std::int64_t>(u64()); }
float BinReader::f32() { float v; std::uint32_t b = u32(); std::memcpy(&v, &b, 4); if (!std::isfinite(v)) throw Error(ErrorCode::malformed, "non-finite float"); return v; }
double BinReader::f64() { double v; std::uint64_t b = u64(); std::memcpy(&v, &b, 8); if (!std::isfinite(v)) throw Error(ErrorCode::malformed, "non-finite double"); return v; }
std::string BinReader::string() { std::uint32_t n = u32(); if (n > payload_end_ - pos_) throw Error(ErrorCode::malformed, "string length exceeds payload"); std::string s(reinterpret_cast<const char*>(data_.data() + pos_), n); pos_ += n; return s; }
std::vector<std::uint8_t> BinReader::bytes(std::size_t n) { check(n); std::vector<std::uint8_t> v(data_.begin() + pos_, data_.begin() + pos_ + n); pos_ += n; return v; }
void BinReader::expect_end(std::size_t expected) {
  if (pos_ != expected) throw Error(ErrorCode::trailing_garbage, "trailing garbage in payload");
}

// ---- snapshot codecs ----
namespace {
Uuid read_uuid(BinReader& r) { auto v = r.bytes(16); std::array<std::uint8_t, 16> a{}; for (std::size_t i = 0; i < 16; ++i) a[i] = v[i]; return Uuid{a}; }

void write_desc(BinWriter& w, const AdapterDescriptor& d) {
  auto wu = [&](const Uuid& u){ const auto& b = u.bytes(); w.bytes(b.data(), b.size()); };
  wu(d.id.uuid()); wu(d.revision.uuid()); wu(d.artifact.uuid());
  w.string(d.artifact_digest);
  w.u8(static_cast<std::uint8_t>(d.kind));
  w.string(d.name);
  wu(d.base_model.uuid()); wu(d.base_model_revision.uuid());
  w.u32(static_cast<std::uint32_t>(d.targets.size()));
  for (const auto& t : d.targets) { w.string(t.name); w.u32(t.in_features); w.u32(t.out_features); w.u32(static_cast<std::uint32_t>(t.shape.size())); for (auto x : t.shape) w.u32(x); }
  w.u32(d.rank);
  w.u8(static_cast<std::uint8_t>(d.dtype));
  w.u64(d.param_count); w.u64(d.param_bytes); w.u64(d.memory_bytes);
  w.string(d.format); w.u32(d.format_version);
  w.u32(static_cast<std::uint32_t>(d.dependencies.size()));
  for (const auto& x : d.dependencies) { w.string(x.name); w.string(x.version); w.string(x.requirement); }
  w.u32(static_cast<std::uint32_t>(d.capabilities.size()));
  for (const auto& x : d.capabilities) { w.string(x.name); w.string(x.value); w.u8(x.required ? 1 : 0); }
  w.u32(static_cast<std::uint32_t>(d.constraints.size()));
  for (const auto& x : d.constraints) { w.string(x.key); w.string(x.value); }
  w.string(d.provenance.source); w.string(d.provenance.author); w.string(d.provenance.timestamp);
  w.string(d.provenance.origin_digest); w.string(d.provenance.notes);
  write_gen(w, d.generation);
  write_gen(w, d.policy_generation);
  w.u8(static_cast<std::uint8_t>(d.validation));
}

static bool is_kind_ok(AdapterKind k) { return k == AdapterKind::generic || k == AdapterKind::lora || k == AdapterKind::full_finetune || k == AdapterKind::prefix_tuning || k == AdapterKind::custom; }
static bool is_dtype_ok(DType t) { return t == DType::unknown || t == DType::f32 || t == DType::f16 || t == DType::bf16 || t == DType::f8_e4m3 || t == DType::f8_e5m2 || t == DType::int8 || t == DType::int4; }
static bool is_validation_ok(ValidationState v) { return v == ValidationState::none || v == ValidationState::validating || v == ValidationState::valid || v == ValidationState::invalid; }

AdapterDescriptor read_desc(BinReader& r) {
  AdapterDescriptor d;
  d.id = AdapterId{read_uuid(r)};
  d.revision = AdapterRevisionId{read_uuid(r)};
  d.artifact = AdapterArtifactId{read_uuid(r)};
  d.artifact_digest = r.string();
  d.kind = static_cast<AdapterKind>(r.u8());
  if (!is_kind_ok(d.kind)) throw Error(ErrorCode::invalid_enum, "invalid adapter kind");
  d.name = r.string();
  d.base_model = BaseModelId{read_uuid(r)};
  d.base_model_revision = ModelRevisionId{read_uuid(r)};
  std::uint32_t nt = r.u32(); if (nt > 100000) throw Error(ErrorCode::malformed, "target count too large");
  for (std::uint32_t i = 0; i < nt; ++i) { TargetModule t; t.name = r.string(); t.in_features = r.u32(); t.out_features = r.u32(); std::uint32_t ns = r.u32(); if (ns > 64) throw Error(ErrorCode::malformed, "shape too large"); for (std::uint32_t j = 0; j < ns; ++j) t.shape.push_back(r.u32()); d.targets.push_back(std::move(t)); }
  d.rank = r.u32();
  d.dtype = static_cast<DType>(r.u8());
  if (!is_dtype_ok(d.dtype)) throw Error(ErrorCode::invalid_enum, "invalid dtype");
  d.param_count = r.u64(); d.param_bytes = r.u64(); d.memory_bytes = r.u64();
  d.format = r.string(); d.format_version = r.u32();
  if (d.format_version == 0) throw Error(ErrorCode::invalid_enum, "format version zero");
  std::uint32_t nd = r.u32(); if (nd > 100000) throw Error(ErrorCode::malformed, "dependency count too large");
  for (std::uint32_t i = 0; i < nd; ++i) { Dependency x; x.name = r.string(); x.version = r.string(); x.requirement = r.string(); d.dependencies.push_back(std::move(x)); }
  std::uint32_t nc = r.u32(); if (nc > 100000) throw Error(ErrorCode::malformed, "capability count too large");
  for (std::uint32_t i = 0; i < nc; ++i) { RuntimeCapability x; x.name = r.string(); x.value = r.string(); x.required = r.u8() != 0; d.capabilities.push_back(std::move(x)); }
  std::uint32_t nx = r.u32(); if (nx > 100000) throw Error(ErrorCode::malformed, "constraint count too large");
  for (std::uint32_t i = 0; i < nx; ++i) { ExecutionConstraint x; x.key = r.string(); x.value = r.string(); d.constraints.push_back(std::move(x)); }
  d.provenance.source = r.string(); d.provenance.author = r.string(); d.provenance.timestamp = r.string();
  d.provenance.origin_digest = r.string(); d.provenance.notes = r.string();
  d.generation = read_gen<gen::adapter>(r);
  d.policy_generation = read_gen<gen::policy>(r);
  if (d.generation.is_none()) throw Error(ErrorCode::invalid_generation_value, "adapter generation is none");
  d.validation = static_cast<ValidationState>(r.u8());
  if (!is_validation_ok(d.validation)) throw Error(ErrorCode::invalid_enum, "invalid validation state");
  return d;
}
}  // namespace

std::vector<std::uint8_t> serialize_adapter(const AdapterDescriptor& d) {
  BinWriter w;
  write_desc(w, d);
  return w.finish();
}

AdapterDescriptor deserialize_adapter(const std::string& bytes) {
  std::span<const std::uint8_t> sp(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
  return deserialize_adapter(sp);
}

AdapterDescriptor deserialize_adapter(std::span<const std::uint8_t> bytes) {
  BinReader r(bytes);
  AdapterDescriptor d = read_desc(r);
  r.expect_end(r.size() - 8);
  return d;
}

std::vector<std::uint8_t> serialize_snapshot(const FabricSnapshot& s) {
  BinWriter w;
  w.u32(FabricSnapshot::kFormat);
  write_gen(w, s.epoch);
  write_gen(w, s.next_adapter_generation);
  write_gen(w, s.next_composition_generation);
  w.u32(static_cast<std::uint32_t>(s.adapters.size()));
  for (const auto& a : s.adapters) write_desc(w, a);
  w.u32(static_cast<std::uint32_t>(s.compositions.size()));
  for (const auto& c : s.compositions) {
    auto wu = [&](const Uuid& u){ const auto& b = u.bytes(); w.bytes(b.data(), b.size()); };
    wu(c.id.uuid());
    w.u32(static_cast<std::uint32_t>(c.members.size()));
    for (const auto& m : c.members) { wu(m.adapter_id.uuid()); wu(m.revision.uuid()); write_gen(w, m.generation); w.u64(m.memory_bytes); w.u8(static_cast<std::uint8_t>(m.kind)); }
    write_gen(w, c.generation);
    wu(c.base_model.uuid()); wu(c.base_model_revision.uuid());
    w.string(c.policy);
    w.u64(c.total_memory_bytes);
    w.string(c.digest); w.string(c.validation_summary);
    w.u8(c.is_valid ? 1 : 0);
  }
  w.u32(static_cast<std::uint32_t>(s.workers.size()));
  for (const auto& wk : s.workers) {
    auto wu = [&](const Uuid& u){ const auto& b = u.bytes(); w.bytes(b.data(), b.size()); };
    wu(wk.id.uuid()); wu(wk.boot.uuid()); w.string(wk.address);
    wu(wk.node.uuid()); wu(wk.device.uuid());
    w.u8(wk.connected ? 1 : 0); write_gen(w, wk.epoch);
    w.u32(static_cast<std::uint32_t>(wk.capabilities.size()));
    for (const auto& cap : wk.capabilities) { w.string(cap.name); w.string(cap.value); }
    w.u64(wk.device_memory_bytes);
    w.u32(static_cast<std::uint32_t>(wk.local_adapters.size()));
    for (const auto& id : wk.local_adapters) wu(id.uuid());
  }
  return w.finish();
}

FabricSnapshot deserialize_snapshot(std::span<const std::uint8_t> bytes) {
  BinReader r(bytes);
  FabricSnapshot s;
  std::uint32_t fmt = r.u32();
  if (fmt != FabricSnapshot::kFormat) throw Error(ErrorCode::unsupported_version, "unsupported snapshot version");
  s.epoch = read_gen<gen::coordinator_epoch>(r);
  s.next_adapter_generation = read_gen<gen::adapter>(r);
  s.next_composition_generation = read_gen<gen::composition>(r);
  std::set<std::string> ids, comps, workers;
  std::uint32_t na = r.u32(); if (na > 100000) throw Error(ErrorCode::malformed, "adapter count too large");
  for (std::uint32_t i = 0; i < na; ++i) { auto d = read_desc(r); if (!ids.insert(d.id.str()).second) throw Error(ErrorCode::duplicate, "duplicate adapter id"); s.adapters.push_back(std::move(d)); }
  std::uint32_t nc = r.u32(); if (nc > 100000) throw Error(ErrorCode::malformed, "composition count too large");
  for (std::uint32_t i = 0; i < nc; ++i) {
    Composition c; c.id = CompositionId{read_uuid(r)};
    std::uint32_t nm = r.u32(); if (nm > 100000) throw Error(ErrorCode::malformed, "member count too large");
    std::uint64_t total = 0;
    for (std::uint32_t j = 0; j < nm; ++j) {
      CompositionMember m; m.adapter_id = AdapterId{read_uuid(r)}; m.revision = AdapterRevisionId{read_uuid(r)};
      m.generation = read_gen<gen::adapter>(r); if (m.generation.is_none()) throw Error(ErrorCode::invalid_generation_value, "member generation none");
      m.memory_bytes = r.u64(); m.kind = static_cast<AdapterKind>(r.u8()); total += m.memory_bytes;
      c.members.push_back(std::move(m));
    }
    if (c.members.empty()) throw Error(ErrorCode::invalid_composition, "empty composition");
    c.generation = read_gen<gen::composition>(r); if (c.generation.is_none()) throw Error(ErrorCode::invalid_generation_value, "composition generation none");
    c.base_model = BaseModelId{read_uuid(r)}; c.base_model_revision = ModelRevisionId{read_uuid(r)};
    c.policy = r.string(); c.total_memory_bytes = r.u64(); c.digest = r.string(); c.validation_summary = r.string();
    c.is_valid = r.u8() != 0;
    if (c.total_memory_bytes != total) throw Error(ErrorCode::invalid_composition, "composition memory accounting mismatch");
    if (CompositionBuilder::compute_digest(c.members, c.policy) != c.digest) throw Error(ErrorCode::invalid_composition, "composition digest mismatch");
    if (!comps.insert(c.id.str()).second) throw Error(ErrorCode::duplicate, "duplicate composition id");
    s.compositions.push_back(std::move(c));
  }
  std::uint32_t nw = r.u32(); if (nw > 100000) throw Error(ErrorCode::malformed, "worker count too large");
  for (std::uint32_t i = 0; i < nw; ++i) {
    WorkerRecord wk; wk.id = WorkerId{read_uuid(r)}; wk.boot = WorkerBootId{read_uuid(r)}; wk.address = r.string();
    wk.node = NodeId{read_uuid(r)}; wk.device = DeviceId{read_uuid(r)}; wk.connected = r.u8() != 0; wk.epoch = read_gen<gen::coordinator_epoch>(r);
    std::uint32_t ncap = r.u32(); if (ncap > 100000) throw Error(ErrorCode::malformed, "capability count too large");
    for (std::uint32_t j = 0; j < ncap; ++j) { WorkerCapability cap; cap.name = r.string(); cap.value = r.string(); wk.capabilities.push_back(std::move(cap)); }
    wk.device_memory_bytes = r.u64();
    std::uint32_t nla = r.u32(); if (nla > 100000) throw Error(ErrorCode::malformed, "local adapter count too large");
    for (std::uint32_t j = 0; j < nla; ++j) wk.local_adapters.push_back(AdapterId{read_uuid(r)});
    if (!workers.insert(wk.id.str()).second) throw Error(ErrorCode::duplicate, "duplicate worker id");
    s.workers.push_back(std::move(wk));
  }
  r.expect_end(r.size() - 8);
  return s;
}

}  // namespace adapter_fabric