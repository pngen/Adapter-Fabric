# Adapter Fabric — Architecture

Copyright 2026 Summon Software Labs. Apache-2.0.

## Design invariants

1. **Canonical vs. ephemeral.** The coordinator holds canonical metadata and
authority; workers hold only ephemeral, process-local adapter state. A worker
death never resurrects readiness, residency, or activation.
2. **Generation, not mutation.** Any change to an adapter, composition,
residency, artifact, or authority yields a new generation, never an in-place
edit. This makes stale replay detectable and rejectable.
3. **Authority is fenced.** `ACTIVE` requires a fully current `AuthorityFence`.
Every stale factor is independently rejectable.
4. **Single-writer exclusivity.** Where required, exactly one current
authoritative activation exists; contention is an explicit `authority_conflict`.
5. **Explainable determinism.** Compatibility, readiness, activation, reuse,
migration, eviction, composition validity, stale rejection, and invalidation
each produce structured text and JSON.

## Layers

| Layer | Header | Role |
|-------|--------|------|
| Identity | `identity.hpp`, `uuid.hpp`, `generation.hpp` | 128-bit strongly-typed ids and monotonic generations |
| Domain | `adapter.hpp`, `compatibility.hpp`, `lifecycle.hpp` | adapter descriptor, compatibility evidence, guarded lifecycle |
| State | `residency.hpp`, `capacity.hpp`, `composition.hpp` | residency records, capacity accounting, immutable compositions |
| Authority | `authority.hpp`, `migration.hpp`, `invalidation.hpp` | fence validation, migration FSM, invalidation |
| Persistence | `persistence.hpp` | strict versioned binary codec, checksum, snapshots |
| Coordination | `fabric.hpp` | canonical registry + authority decision point |
| Transport | `protocol.hpp`, `transport.hpp` | framed TCP messages with strict codec |
| Distributed | `coordinator.hpp`, `worker.hpp` | coordinator service, worker service |
| Accelerator | `cuda.hpp` | CUDA backend + LoRA-like kernel |
| Proofs | `proof.hpp` | multiprocess, CUDA, and benchmark drivers |

## Coordinator + worker model

`af_coordinator` hosts a `Fabric` and a `CoordinatorServer` that accepts
connections. Workers connect and send `wk_register` (worker id, `WorkerBootId`,
capabilities, node/device). A control client (`af_cli` or the multiprocess proof)
sends `ctl_*` messages. The coordinator validates every mutation against
`check_fence`, forwards worker commands over the worker socket (never under the
global state lock), and correlates replies via a pending-request table keyed by
`req_id`.

### Workflow of a load + activate

1. `ctl_register_adapter` deserializes a `AdapterDescriptor`, assigns a fresh
`AdapterGeneration`, and stores canonical metadata.
2. `ctl_validate` evaluates it against a `CompatibilityTarget`, returning
structured evidence.
3. `ctl_load` decodes the fence, calls `check_fence` (rejects stale
boot/epoch/generation/attempt before forwarding), creates an instance, forwards
`wk_load` to the worker, and marks the residency ready on acknowledgement.
4. `ctl_activate` re-checks the fence, forwards `wk_activate`, and binds the
authoritative activation (`bind_authority`).
5. `ctl_deactivate`/`ctl_evict` release execution authority and capacity
accounting.

### Process-local ephemerality

Each worker derives a deterministic `WorkerBootId` from a seed. When a worker
re-registers with a new boot, `on_worker_restart` invalidates all authority and
residency held under the old boot. When a socket closes, `on_worker_down` clears
the worker's authority and marks it disconnected, so a dead worker's adapter
state is never treated as resident or active.

## Persistence format

Snapshots are versioned (`kFormat = 1`), little-endian, and covered by an
8-byte FNV-1a checksum. The reader rejects truncation, checksum mismatch,
trailing garbage, duplicate ids, invalid enums/generations/identities, non-finite
floats, impossible state, and inconsistent composition membership. Recovery
populates canonical metadata and generations only; it never restores activation.

## Threading

The coordinator uses one socket-pair per connection thread for network I/O and a
single `state_mc_` mutex for authority/state mutation. Network I/O never happens
while holding `state_mc_`; each socket's writer is serialized by its own mutex.
The control path's request/response bridge is correlation-safe under concurrent
connection threads.

## CUDA backend

`cuda_impl.cu` provides the device kernels and runtime calls, `cuda_backend.cpp`
the host wrapper, and `cuda.hpp` the public API. When the CUDA runtime is not
linked the API degrades to `not_available` and the proof reports a clear reason.
The kernel implements `y = x + scale * W_down (W_up x)` for a low-rank
transformation, verified against a CPU reference.

## Build layout

`include/adapter_fabric/` public headers; `src/` implementation; `apps/`
`af_cli`, `af_coordinator`, `af_worker`; `tests/` ctest suite; `examples/`;
`benchmarks/`; `consumer/` the `find_package` consumer; `cmake/` package config;
`scripts/` environment/build wrappers.
