// Adapter Fabric — fabric implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/fabric.hpp"

namespace adapter_fabric {

Fabric::Fabric() {
  epoch_ = CoordinatorEpoch::none();
  next_adapter_gen_ = AdapterGeneration{1};
  next_composition_gen_ = CompositionGeneration{1};
  for (auto loc : {ResidencyLocation::device_memory, ResidencyLocation::pinned_host,
                   ResidencyLocation::pageable_host, ResidencyLocation::process_local,
                   ResidencyLocation::node_local, ResidencyLocation::persistent_storage}) {
    if (capacity_.limit(loc) == 0) capacity_.set_limit(loc, 1ULL << 40);
  }
}

Fabric::Fabric(std::map<ResidencyLocation, std::uint64_t> limits) : capacity_(std::move(limits)) {
  epoch_ = CoordinatorEpoch::none();
  next_adapter_gen_ = AdapterGeneration{1};
  next_composition_gen_ = CompositionGeneration{1};
}

AdapterDescriptor Fabric::register_adapter(AdapterDescriptor d) {
  if (d.id.is_nil()) throw Error(ErrorCode::invalid_identity, "adapter id is nil");
  if (d.base_model.is_nil()) throw Error(ErrorCode::invalid_identity, "base model id is nil");
  if (d.base_model_revision.is_nil()) throw Error(ErrorCode::invalid_identity, "base model revision id is nil");
  if (d.artifact_digest.empty()) throw Error(ErrorCode::invalid_argument, "adapter has no artifact digest");
  if (d.memory_bytes == 0) throw Error(ErrorCode::invalid_argument, "adapter declares zero memory footprint");
  d.generation = next_adapter_gen_;
  next_adapter_gen_ = next_adapter_gen_.next();
  adapters_[d.id.str()] = d;
  return d;
}

bool Fabric::has_adapter(AdapterId id) const { return adapters_.find(id.str()) != adapters_.end(); }
const AdapterDescriptor* Fabric::adapter(AdapterId id) const {
  auto it = adapters_.find(id.str());
  return it == adapters_.end() ? nullptr : &it->second;
}
std::vector<AdapterDescriptor> Fabric::adapters() const {
  std::vector<AdapterDescriptor> out;
  for (const auto& kv : adapters_) out.push_back(kv.second);
  return out;
}

CompatibilityReport Fabric::validate(AdapterId id, const CompatibilityTarget& target) const {
  const AdapterDescriptor* d = adapter(id);
  if (!d) throw Error(ErrorCode::not_found, "unknown adapter");
  return evaluate_compatibility(*d, target);
}

Composition Fabric::publish_composition(const std::vector<AdapterId>& ordered, std::string policy) {
  if (ordered.empty()) throw Error(ErrorCode::invalid_composition, "empty composition");
  const AdapterDescriptor* first = adapter(ordered[0]);
  if (!first) throw Error(ErrorCode::not_found, "unknown adapter in composition");
  CompositionBuilder b(first->base_model, first->base_model_revision, std::move(policy), next_composition_gen_);
  for (const auto& id : ordered) {
    const AdapterDescriptor* d = adapter(id);
    if (!d) throw Error(ErrorCode::not_found, "unknown adapter in composition");
    b.add(*d);
  }
  Composition c = b.build();
  compositions_[c.id.str()] = c;
  next_composition_gen_ = next_composition_gen_.next();
  return c;
}

const Composition* Fabric::composition(CompositionId id) const {
  auto it = compositions_.find(id.str());
  return it == compositions_.end() ? nullptr : &it->second;
}
std::vector<Composition> Fabric::compositions() const {
  std::vector<Composition> out;
  for (const auto& kv : compositions_) out.push_back(kv.second);
  return out;
}

void Fabric::register_worker(WorkerRecord rec) { workers_[rec.id.str()] = std::move(rec); }
const WorkerRecord* Fabric::worker(WorkerId id) const {
  auto it = workers_.find(id.str());
  return it == workers_.end() ? nullptr : &it->second;
}
std::vector<WorkerRecord> Fabric::workers() const {
  std::vector<WorkerRecord> out;
  for (const auto& kv : workers_) out.push_back(kv.second);
  return out;
}

AdapterInstanceId Fabric::create_instance(AdapterId adapter_id, WorkerId worker, WorkerBootId boot, DeviceId device, std::uint64_t bytes) {
  const AdapterDescriptor* d = adapter(adapter_id);
  if (!d) throw Error(ErrorCode::not_found, "unknown adapter");
  AdapterInstanceId inst = AdapterInstanceId::generate();
  ResidencyRecord rec;
  rec.instance = inst;
  rec.adapter_id = adapter_id;
  rec.adapter_generation = d->generation;
  rec.residency_generation = ResidencyGeneration{1};
  rec.location = ResidencyLocation::device_memory;
  rec.bytes = bytes;
  rec.worker = worker;
  rec.boot = boot;
  rec.epoch = epoch_;
  rec.device = device;
  residencies_[inst.str()] = rec;
  AdapterAuthorityRecord ar;
  ar.adapter = adapter_id;
  ar.instance = inst;
  ar.lifecycle = Lifecycle(LifecycleState::loading);
  ar.residency_generation = ResidencyGeneration{1};
  ar.fence = AuthorityFence{};
  ar.fence.adapter = adapter_id; ar.fence.epoch = epoch_;
  ar.fence.adapter_generation = d->generation; ar.fence.base_model_revision = d->base_model_revision;
  ar.fence.worker = worker; ar.fence.boot = boot; ar.fence.device = device;
  ar.fence.residency_generation = ResidencyGeneration{1};
  ar.fence.composition_generation = CompositionGeneration{1};
  ar.fence.artifact_generation = ArtifactGeneration{1};
  authority_[adapter_id.str()] = ar;
  return inst;
}

bool Fabric::has_instance(AdapterInstanceId inst) const { return residencies_.find(inst.str()) != residencies_.end(); }
const AdapterAuthorityRecord* Fabric::authority_record(AdapterId adapter) const {
  auto it = authority_.find(adapter.str());
  return it == authority_.end() ? nullptr : &it->second;
}
ResidencyRecord* Fabric::residency(AdapterInstanceId inst) {
  auto it = residencies_.find(inst.str());
  return it == residencies_.end() ? nullptr : &it->second;
}

AuthorityCheck Fabric::check_fence(const AuthorityFence& fence) const {
  const AdapterDescriptor* d = adapter(fence.adapter);
  if (!d || fence.adapter.is_nil()) return AuthorityCheck{AuthorityVerdict::unknown, "unknown or nil adapter"};
  if (fence.epoch != epoch_) return AuthorityCheck{AuthorityVerdict::stale_epoch, "coordinator epoch mismatch"};
  if (fence.adapter_generation != d->generation) return AuthorityCheck{AuthorityVerdict::stale_adapter_generation, "adapter generation mismatch"};
  if (fence.base_model_revision != d->base_model_revision) return AuthorityCheck{AuthorityVerdict::stale_base_model, "base model revision mismatch"};
  const WorkerRecord* w = worker(fence.worker);
  if (!w) return AuthorityCheck{AuthorityVerdict::stale_worker, "unknown worker"};
  if (!w->connected) return AuthorityCheck{AuthorityVerdict::stale_worker, "worker not connected"};
  if (fence.worker.is_nil() || fence.boot.is_nil() || fence.attempt.is_nil())
    return AuthorityCheck{AuthorityVerdict::unknown, "incomplete fence"};
  if (fence.boot != w->boot) return AuthorityCheck{AuthorityVerdict::stale_boot, "worker boot mismatch"};
  auto ai = current_attempt_.find(fence.adapter.str());
  if (ai != current_attempt_.end() && ai->second != fence.attempt) return AuthorityCheck{AuthorityVerdict::stale_attempt, "attempt mismatch"};
  const AdapterAuthorityRecord* ar = authority_record(fence.adapter);
  const ResidencyGeneration rgen = ar ? ar->fence.residency_generation : ResidencyGeneration{1};
  const CompositionGeneration cgen = ar ? ar->fence.composition_generation : CompositionGeneration{1};
  const ArtifactGeneration agen = ar ? ar->fence.artifact_generation : ArtifactGeneration{1};
  if (fence.residency_generation != rgen) return AuthorityCheck{AuthorityVerdict::stale_residency_generation, "residency generation mismatch"};
  if (fence.composition_generation != cgen) return AuthorityCheck{AuthorityVerdict::stale_composition_generation, "composition generation mismatch"};
  if (fence.artifact_generation != agen) return AuthorityCheck{AuthorityVerdict::stale_artifact_generation, "artifact generation mismatch"};
  return AuthorityCheck{AuthorityVerdict::authorized, "fence current"};
}

AuthorityCheck Fabric::bind_authority(AdapterId id, const AuthorityFence& fence) {
  AuthorityCheck check = check_fence(fence);
  if (!check.authorized()) return check;
  if (active_fence_ && active_fence_->adapter != id) {
    throw Error(ErrorCode::authority_conflict, "another adapter holds exclusive execution authority");
  }
  auto it = authority_.find(id.str());
  if (it == authority_.end()) throw Error(ErrorCode::not_found, "no authority record for adapter");
  it->second.fence = fence;
  it->second.has_authority = true;
  it->second.active = true;
  it->second.lifecycle.set_state(LifecycleState::active);
  current_attempt_[id.str()] = fence.attempt;
  active_fence_ = fence;
  return check;
}


AuthorityCheck Fabric::reuse_safe(const AuthorityFence& fence) const {
  AuthorityCheck check = check_fence(fence);
  if (!check.authorized()) return check;
  const AdapterAuthorityRecord* ar = authority_record(fence.adapter);
  if (!ar || !ar->has_authority) return AuthorityCheck{AuthorityVerdict::unknown, "no authority for reuse"};
  const auto rit = residencies_.find(ar->instance.str());
  if (rit != residencies_.end() && rit->second.ready) return AuthorityCheck{AuthorityVerdict::authorized, "reuse permitted: current and resident"};
  return AuthorityCheck{AuthorityVerdict::unknown, "reuse denied: not resident/ready"};
}

InvalidationRecord Fabric::invalidate(AdapterId id, InvalidationTrigger trigger, std::string reason) {
  InvalidationRecord rec;
  rec.trigger = trigger;
  rec.adapter = id;
  rec.reason = std::move(reason);
  rec.epoch = epoch_;
  const AdapterDescriptor* d = adapter(id);
  if (d) rec.prior_generation = d->generation;
  invalidation_log_.push_back(rec);
  auto it = authority_.find(id.str());
  if (it != authority_.end()) {
    it->second.active = false;
    it->second.has_authority = false;
    if (it->second.lifecycle.state() != LifecycleState::invalidated)
      it->second.lifecycle.set_state(LifecycleState::invalidated);
  }
  if (active_fence_ && active_fence_->adapter == id) active_fence_.reset();
  return rec;
}

bool Fabric::evict(AdapterInstanceId inst, std::string& reason) {
  auto it = residencies_.find(inst.str());
  if (it == residencies_.end()) { reason = "no such instance"; return false; }
  if (it->second.active_refs > 0) { reason = "active references"; return false; }
  if (it->second.pin_count > 0 || it->second.protected_) { reason = "protected or pinned"; return false; }
  residencies_.erase(it);
  reason = "evicted";
  return true;
}

void Fabric::pin(AdapterInstanceId inst) {
  auto it = residencies_.find(inst.str());
  if (it == residencies_.end()) throw Error(ErrorCode::not_found, "no such instance");
  it->second.pin_count += 1;
  it->second.protected_ = true;
}

void Fabric::unpin(AdapterInstanceId inst) {
  auto it = residencies_.find(inst.str());
  if (it == residencies_.end()) throw Error(ErrorCode::not_found, "no such instance");
  if (it->second.pin_count > 0) it->second.pin_count -= 1;
  if (it->second.pin_count == 0) it->second.protected_ = false;
}

FabricSnapshot Fabric::snapshot() const {
  FabricSnapshot s;
  s.epoch = epoch_;
  s.next_adapter_generation = next_adapter_gen_;
  s.next_composition_generation = next_composition_gen_;
  for (const auto& kv : adapters_) s.adapters.push_back(kv.second);
  for (const auto& kv : compositions_) s.compositions.push_back(kv.second);
  for (const auto& kv : workers_) s.workers.push_back(kv.second);
  return s;
}

void Fabric::recover(const FabricSnapshot& s) {
  epoch_ = s.epoch;
  next_adapter_gen_ = s.next_adapter_generation;
  next_composition_gen_ = s.next_composition_generation;
  adapters_.clear();
  for (const auto& a : s.adapters) adapters_[a.id.str()] = a;
  compositions_.clear();
  for (const auto& c : s.compositions) compositions_[c.id.str()] = c;
  workers_.clear();
  for (const auto& w : s.workers) workers_[w.id.str()] = w;
}

void Fabric::on_worker_down(WorkerId id) {
  for (auto& kv : authority_) {
    if (kv.second.fence.worker == id) {
      kv.second.active = false;
      kv.second.has_authority = false;
      kv.second.lifecycle.set_state(LifecycleState::invalidated);
      current_attempt_.erase(kv.first);
    }
  }
  for (auto& kv : residencies_) {
    if (kv.second.worker == id) { kv.second.ready = false; }
  }
  auto it = workers_.find(id.str());
  if (it != workers_.end()) it->second.connected = false;
  if (active_fence_ && active_fence_->worker == id) active_fence_.reset();
}

void Fabric::on_worker_restart(WorkerId id, WorkerBootId new_boot) {
  for (auto& kv : authority_) {
    if (kv.second.fence.worker == id && kv.second.fence.boot != new_boot) {
      kv.second.active = false;
      kv.second.has_authority = false;
      kv.second.lifecycle.set_state(LifecycleState::invalidated);
      current_attempt_.erase(kv.first);
    }
  }
  for (auto& kv : residencies_) {
    if (kv.second.worker == id && kv.second.boot != new_boot) {
      kv.second.ready = false;
      kv.second.migration = MigrationState::none;
    }
  }
  if (active_fence_ && active_fence_->worker == id && active_fence_->boot != new_boot) active_fence_.reset();
}

}  // namespace adapter_fabric