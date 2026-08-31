// Adapter Fabric — canonical fabric orchestration.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "adapter_fabric/adapter.hpp"
#include "adapter_fabric/authority.hpp"
#include "adapter_fabric/capacity.hpp"
#include "adapter_fabric/compatibility.hpp"
#include "adapter_fabric/composition.hpp"
#include "adapter_fabric/inventory.hpp"
#include "adapter_fabric/invalidation.hpp"
#include "adapter_fabric/lifecycle.hpp"
#include "adapter_fabric/migration.hpp"
#include "adapter_fabric/persistence.hpp"
#include "adapter_fabric/residency.hpp"
#include "adapter_fabric/explain.hpp"

namespace adapter_fabric {

// A published activation fence together with the durable record it authorizes.
struct AdapterAuthorityRecord {
  AdapterId adapter;
  AdapterInstanceId instance;
  Lifecycle lifecycle;
  ResidencyGeneration residency_generation;
  AuthorityFence fence;
  bool has_authority = false;
  bool active = false;
};

// The canonical, coordinator-side registry and authority decision point. It
// deliberately never holds process-local accelerator state; that is ephemeral
// and lives on workers. The Fabric decides what is compatible, current, and
// safe to execute.
class Fabric {
 public:
  Fabric();
  explicit Fabric(std::map<ResidencyLocation, std::uint64_t> limits);

  // --- epochs ---
  CoordinatorEpoch epoch() const noexcept { return epoch_; }
  CoordinatorEpoch advance_epoch() noexcept { epoch_ = epoch_.next(); return epoch_; }

  // --- adapters ---
  AdapterDescriptor register_adapter(AdapterDescriptor d);
  bool has_adapter(AdapterId id) const;
  const AdapterDescriptor* adapter(AdapterId id) const;
  std::vector<AdapterDescriptor> adapters() const;

  // --- validation / compatibility ---
  CompatibilityReport validate(AdapterId id, const CompatibilityTarget& target) const;

  // --- composition ---
  Composition publish_composition(const std::vector<AdapterId>& ordered, std::string policy);
  const Composition* composition(CompositionId id) const;
  std::vector<Composition> compositions() const;

  // --- workers ---
  void register_worker(WorkerRecord rec);
  const WorkerRecord* worker(WorkerId id) const;
  std::vector<WorkerRecord> workers() const;

  // --- residency / authority ---
  AdapterInstanceId create_instance(AdapterId adapter, WorkerId worker, WorkerBootId boot, DeviceId device, std::uint64_t bytes);
  bool has_instance(AdapterInstanceId inst) const;
  const AdapterAuthorityRecord* authority_record(AdapterId adapter) const;
  ResidencyRecord* residency(AdapterInstanceId inst);

  // Validate a fence as current relative to the fabric's epoch/generations.
  AuthorityCheck check_fence(const AuthorityFence& fence) const;

  // Bind (or reject) the single authoritative activation. Returns the check.
  AuthorityCheck bind_authority(AdapterId adapter, const AuthorityFence& fence);

  // Is reuse of an existing instance safe given a candidate fence?
  AuthorityCheck reuse_safe(const AuthorityFence& fence) const;

  // --- worker restart ---
  // Invalidate all authority/residency held by a worker under a prior boot.
  void on_worker_restart(WorkerId id, WorkerBootId new_boot);
  void on_worker_down(WorkerId id);

  // --- invalidation / eviction ---
  InvalidationRecord invalidate(AdapterId id, InvalidationTrigger trigger, std::string reason);
  bool evict(AdapterInstanceId inst, std::string& reason);
  void pin(AdapterInstanceId inst);
  void unpin(AdapterInstanceId inst);

  // --- capacity ---
  CapacityAccountant& capacity() noexcept { return capacity_; }
  CapacityAccountant& residency_capacity() noexcept { return capacity_; }

  // --- persistence ---
  FabricSnapshot snapshot() const;
  void recover(const FabricSnapshot& s);

  // --- invalidation log ---
  const std::vector<InvalidationRecord>& invalidation_log() const noexcept { return invalidation_log_; }

 private:
  CoordinatorEpoch epoch_;
  AdapterGeneration next_adapter_gen_;
  CompositionGeneration next_composition_gen_;
  std::map<std::string, AdapterDescriptor> adapters_;
  std::map<std::string, Composition> compositions_;
  std::map<std::string, WorkerRecord> workers_;
  std::map<std::string, AdapterAuthorityRecord> authority_;
  std::map<std::string, ResidencyRecord> residencies_;
  std::vector<InvalidationRecord> invalidation_log_;
  CapacityAccountant capacity_;
  std::optional<AuthorityFence> active_fence_;
  std::map<std::string, AttemptId> current_attempt_;
};

}  // namespace adapter_fabric