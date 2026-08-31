// Adapter Fabric — deterministic, explainable compatibility.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include "adapter_fabric/adapter.hpp"
#include "adapter_fabric/identity.hpp"

namespace adapter_fabric {

enum class FactorStatus : std::uint8_t {
  accepted = 0,
  rejected = 1,
  missing = 2,
};

const char* to_string(FactorStatus s) noexcept;

// One factor of a compatibility decision, carrying evidence rather than a
// bare boolean so consumers can explain acceptance/rejection deterministically.
struct CompatibilityFactor {
  std::string name;
  FactorStatus status = FactorStatus::accepted;
  std::string detail;   // what was compared / the evidence
  std::string reason;   // exact reason when rejected
};

// The target an adapter is being validated against: a base model revision,
// a device/backend, and a policy generation. All comparisons are deterministic.
struct CompatibilityTarget {
  BaseModelId base_model;
  ModelRevisionId base_model_revision;
  std::string architecture;          // e.g. "llama"
  std::string device_arch;          // e.g. "sm_120", "cpu"
  DeviceId device;
  std::map<std::string, std::string> runtime_capabilities;  // descriptor -> value
  std::map<std::string, std::string> policy;               // policy requirements
  PolicyGeneration policy_generation;
};

struct CompatibilityReport {
  bool compatible = false;
  std::vector<CompatibilityFactor> factors;
  AdapterGeneration adapter_generation;
  PolicyGeneration policy_generation;
  ModelRevisionId base_model_revision;
  std::string summary;     // deterministic human/JSON text

  bool has_rejections() const noexcept {
    for (const auto& f : factors) if (f.status == FactorStatus::rejected) return true;
    return false;
  }
  std::string first_rejection_reason() const {
    for (const auto& f : factors) if (f.status == FactorStatus::rejected) return f.reason;
    return "";
  }
};

// Deterministic compatibility evaluation against a target.
CompatibilityReport evaluate_compatibility(const AdapterDescriptor& adapter, const CompatibilityTarget& target);

}  // namespace adapter_fabric
