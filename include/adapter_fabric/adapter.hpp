// Adapter Fabric — adapter domain model.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "adapter_fabric/generation.hpp"
#include "adapter_fabric/identity.hpp"

namespace adapter_fabric {

// The kind of adapter transformation. The runtime is not hard-wired to one
// format; LoRA is the canonical concrete kind, and others are represented
// through the same descriptor shape.
enum class AdapterKind : std::uint8_t {
  generic = 0,
  lora = 1,
  full_finetune = 2,
  prefix_tuning = 3,
  custom = 255,
};

const char* to_string(AdapterKind kind) noexcept;

enum class DType : std::uint8_t {
  unknown = 0,
  f32,
  f16,
  bf16,
  f8_e4m3,
  f8_e5m2,
  int8,
  int4,
};

const char* to_string(DType dtype) noexcept;
std::size_t dtype_bytes(DType dtype) noexcept;

// Rough validation evidence of an artifact. READY requires VALID.
enum class ValidationState : std::uint8_t {
  none = 0,
  validating = 1,
  valid = 2,
  invalid = 3,
};

const char* to_string(ValidationState v) noexcept;

// A target module the adapter attaches to. Carries the tensor shape so
// compatibility can be checked against an actual base model.
struct TargetModule {
  std::string name;
  std::uint32_t in_features = 0;
  std::uint32_t out_features = 0;
  std::vector<std::uint32_t> shape;
};

// A declared dependency of the adapter artifact.
struct Dependency {
  std::string name;
  std::string version;
  std::string requirement;  // e.g. "exact", ">=", range
};

// A required runtime capability (e.g. device "sm_120", dtype "fp16").
struct RuntimeCapability {
  std::string name;
  std::string value;
  bool required = true;
};

// An optional execution constraint (key/value).
struct ExecutionConstraint {
  std::string key;
  std::string value;
};

struct Provenance {
  std::string source;
  std::string author;
  std::string timestamp;
  std::string origin_digest;
  std::string notes;
};

// A complete, immutable adapter descriptor. Represents *canonical metadata* for
// one adapter revision/artifact; it never encodes ephemeral process-local state.
struct AdapterDescriptor {
  AdapterId id;
  AdapterRevisionId revision;
  AdapterArtifactId artifact;
  std::string artifact_digest;  // hex-encoded content digest
  AdapterKind kind = AdapterKind::generic;
  std::string name;
  BaseModelId base_model;
  ModelRevisionId base_model_revision;
  std::vector<TargetModule> targets;
  std::uint32_t rank = 0;
  DType dtype = DType::unknown;
  std::uint64_t param_count = 0;
  std::uint64_t param_bytes = 0;
  std::uint64_t memory_bytes = 0;
  std::string format;
  std::uint32_t format_version = 1;
  std::vector<Dependency> dependencies;
  std::vector<RuntimeCapability> capabilities;
  std::vector<ExecutionConstraint> constraints;
  Provenance provenance;
  AdapterGeneration generation;
  PolicyGeneration policy_generation;
  ValidationState validation = ValidationState::none;
};

// The registry key for a concrete adapter instance on a worker. Distinct from
// the canonical descriptor: an instance is a worker-local runtime embodiment.
struct AdapterInstance {
  AdapterInstanceId id;
  AdapterId adapter_id;
  AdapterRevisionId revision;
  AdapterGeneration generation;
  WorkerId worker;
  WorkerBootId boot;
  DeviceId device;
  std::uint64_t memory_bytes = 0;
};

}  // namespace adapter_fabric
