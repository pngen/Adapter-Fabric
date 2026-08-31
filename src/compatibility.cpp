// Adapter Fabric — compatibility implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/compatibility.hpp"

namespace adapter_fabric {

const char* to_string(FactorStatus s) noexcept {
  switch (s) {
    case FactorStatus::accepted: return "accepted";
    case FactorStatus::rejected: return "rejected";
    case FactorStatus::missing: return "missing";
  }
  return "unknown";
}

CompatibilityReport evaluate_compatibility(const AdapterDescriptor& a, const CompatibilityTarget& t) {
  CompatibilityReport r;
  r.adapter_generation = a.generation;
  r.policy_generation = t.policy_generation;
  r.base_model_revision = t.base_model_revision;

  auto acc = [&](const char* name, std::string detail) { r.factors.push_back({name, FactorStatus::accepted, std::move(detail), ""}); };
  auto rej = [&](const char* name, std::string detail, std::string reason) { r.factors.push_back({name, FactorStatus::rejected, std::move(detail), std::move(reason)}); };
  auto miss = [&](const char* name, std::string detail) { r.factors.push_back({name, FactorStatus::missing, std::move(detail), ""}); };

  if (a.base_model == t.base_model && !a.base_model.is_nil()) {
    acc("base_model", "adapter base model matches target");
  } else if (a.base_model.is_nil()) {
    rej("base_model", "adapter declares no base model", "nil base model identity");
  } else {
    rej("base_model", "base model ids differ", "adapter targets a different base model");
  }

  if (a.base_model_revision == t.base_model_revision && !a.base_model_revision.is_nil()) {
    acc("base_model_revision", "adapter revision matches target");
  } else if (a.base_model_revision.is_nil()) {
    rej("base_model_revision", "adapter declares no model revision", "nil model revision identity");
  } else {
    rej("base_model_revision", "model revisions differ", "adapter is pinned to a different model revision");
  }

  if (a.targets.empty()) {
    rej("targets", "adapter declares no target modules", "no target modules");
  } else {
    bool bad = false;
    for (const auto& x : a.targets) if (x.in_features == 0 || x.out_features == 0) bad = true;
    if (bad) rej("targets", "a target has zero in/out features", "invalid target dimensions");
    else acc("targets", "adapter declares " + std::to_string(a.targets.size()) + " target module(s)");
  }

  if (a.kind == AdapterKind::lora || a.kind == AdapterKind::generic) {
    if (a.rank > 0) acc("rank", "adapter rank " + std::to_string(a.rank) + ">0");
    else rej("rank", "adapter rank is zero", "zero rank is not a valid low-rank adapter");
  } else {
    acc("rank", "non-low-rank adapter; rank not applicable");
  }

  if (a.dtype == DType::unknown) {
    rej("dtype", "adapter dtype unknown", "no dtype declared");
  } else {
    const bool half = t.runtime_capabilities.count("fp16") > 0 && t.runtime_capabilities.at("fp16") == "supported";
    if (a.dtype == DType::f16 || a.dtype == DType::bf16) {
      if (half) acc("dtype", "half precision supported by target");
      else rej("dtype", "half dtype requested", "target does not declare fp16/bf16 support");
    } else if (a.dtype == DType::f32) {
      acc("dtype", "f32 dtype always accepted");
    } else {
      rej("dtype", "dtype not covered by backend", "unsupported dtype for this target");
    }
  }

  if (a.capabilities.empty()) {
    miss("device_capability", "adapter declares no runtime capabilities");
  } else {
    for (const auto& c : a.capabilities) {
      auto it = t.runtime_capabilities.find(c.name);
      if (it == t.runtime_capabilities.end()) {
        if (c.required) rej("device_capability", "missing capability " + c.name, "required capability not present on target");
        else acc("device_capability", "optional capability " + c.name + " absent");
      } else if (it->second == c.value) {
        acc("device_capability", "capability " + c.name + " satisfied");
      } else {
        rej("device_capability", "capability mismatch " + c.name, "target differs from required");
      }
    }
  }

  if (a.dependencies.empty()) {
    acc("dependencies", "no dependencies declared");
  } else {
    for (const auto& dd : a.dependencies) {
      if (!dd.version.empty()) acc("dependency", "dependency " + dd.name + " declared");
      else rej("dependency", "dependency " + dd.name + " has no version", "unversioned dependency");
    }
  }

  if (a.artifact_digest.empty()) {
    rej("artifact", "adapter has no artifact digest", "missing digest");
  } else {
    acc("artifact", "artifact digest present");
  }

  if (a.validation != ValidationState::valid) {
    miss("validation", "adapter not yet validated; revalidate before load");
  } else {
    acc("validation", "adapter artifact validated");
  }

  r.compatible = !r.has_rejections();
  r.summary = r.compatible ? "compatible" : ("incompatible: " + r.first_rejection_reason());
  return r;
}

}  // namespace adapter_fabric
