# Adapter Fabric 1.0.0

Copyright 2026 Summon Software Labs. Apache-2.0.

Adapter Fabric is a real systems runtime that governs adapter-specific runtime
state and execution authority. It answers one question: *how should model
adapters be identified, validated, composed, placed, activated, migrated,
reused, invalidated, and governed so the serving system always knows which
adapter state is compatible, current, resident, authoritative, and safe to
execute?*

It is not a metadata wrapper, a generic model cache, a resident set manager, or
monitoring. It builds and keeps invariant, under concurrency, an explicit
record of which adapter artifact/revision/generation exists, which base model
it attaches to, where its state resides, its guarded lifecycle state, which
composition is authorized, which generation is current, which worker/device
holds valid runtime-local state, and whether reuse or activation is safe.

## Systems boundary

The Fabric governs *adapter-specific* state and authority. It does not schedule
models, size GPU residency, or emit metrics. The boundary is explicit: canonical
metadata and authority live in the coordinator; process-local and device-local
adapter state is always ephemeral and is never resurrected across a worker
restart.

## Core model

Strongly typed 128-bit identities (`AdapterId`, `CompositionId`, `WorkerId`,
`WorkerBootId`, `AttemptId`, ...) round-trip deterministically and never
silently collapse to null. Separate monotonic generations (`AdapterGeneration`,
`ResidencyGeneration`, `CompositionGeneration`, `ArtifactGeneration`,
`AuthorityGeneration`) make a *changed* adapter, composition, residency, or
authority a new generation rather than a mutation.

The adapter domain model (`AdapterDescriptor`) is a generic abstraction: it
carries adapter identity, revision, artifact digest, family/kind (LoRA is the
canonical concrete kind but nothing is hard-wired to it), base-model and
model-revision compatibility, target modules and shapes, rank, dtype, parameter
and memory footprint, format/version, dependencies, runtime capabilities,
constraints, provenance, generation, freshness and validation state.

## Authority model

An execution-relevant activation binds a *fence*: adapter/composition id,
adapter generation, composition generation, residency generation, artifact
generation, base-model revision, coordinator epoch, worker id, worker boot id,
attempt id, and device/node identity. Any stale component is rejected before it
can promote adapter state into execution authority. Where policy requires
exclusivity, exactly one current authoritative activation exists; concurrent
replicas must each carry independent valid authority.

## Adapter lifecycle

A guarded lifecycle `DECLARED -> VALIDATING -> VALID -> LOADING -> LOADED ->
READY -> ACTIVE -> DEACTIVATING -> MIGRATING -> STALE -> INVALIDATED ->
EVICTING -> EVICTED -> FAILED -> RETIRED` is enforced by an explicit allow-list.
`ACTIVE` means execution authority exists, not merely that bytes are resident.
Invalid transitions fail deterministically and leave state unchanged.

## Compatibility

Compatibility is deterministic and explainable. It is validated against base
model identity, model revision, adapter revision, artifact generation,
architecture, target modules, tensor shapes, dtype, runtime/backend capability,
device capability, composition constraints, dependency versions, and policy
generation. It returns structured evidence (accepted/rejected/missing factors,
exact incompatibility reason, provenance, generation used), not a bare boolean.

## Composition

`AdapterSet`/`Composition` members are ordered and immutable once published
under a generation. Duplicate membership, generation mismatch, base-model
mismatch, conflicting target semantics, stale members, incomplete compositions,
and unauthorized mutation are rejected. A changed composition gets a new
generation and a new digest of the complete authorized composition.

## Residency

Residency is tracked per adapter across device memory, pinned host, pageable
host, process-local, node-local, and persistent storage — with byte accounting,
residency generation, adapter generation, worker boot, coordinator epoch,
readiness, active reference count, pin/protection state, migration state,
freshness, and compatibility evidence. Loading is transactional (`plan -> reserve
-> acquire -> validate -> allocate -> transfer -> verify -> publish -> commit`)
and failure before commit rolls back partial state while preserving the prior
authoritative state. Capacity accounting proves no overcommit, no double
reservation/release, no leaks, no eviction of protected/active state, and an
exact return to baseline after teardown.

## Persistence / recovery

Strict versioned persistence uses deterministic serialization and content
checksums. Malformed lengths, truncation, checksum mismatch, duplicate ids,
invalid enums, invalid generations, invalid identity encodings, NaN/Inf,
integer overflow, impossible state transitions, inconsistent composition
membership, incompatible base-model references, trailing garbage, and
unsupported versions are all rejected. Recovery restores durable canonical
metadata only — it never silently resurrects execution authority.

## Distributed proof

A real coordinator and worker path runs over framed TCP with checksums and
strict decoding. A coordinator + two worker OS processes prove the full
sequence: registration, `WorkerBootId`, epoch handling, inventory, load,
readiness, activation, migration acknowledgement, invalidation, and
completion/error. Killing one worker as an OS process proves process-local and
device-local state is not resurrected; a fresh boot id is required. Replaying a
stale boot/epoch/attempt/generation is rejected on the real transport. Network
I/O is never performed while holding the global coordinator state lock, and
socket writer serialization is safe under concurrency.

## CUDA proof

The RTX 5090 (sm_120) is used for a real accelerator proof: cold -> allocate
real device memory -> transfer representative adapter state -> execute a real
CUDA kernel that consumes adapter state -> verify against an exact CPU
reference -> warm reuse without redundant reload -> invalidate/evict -> release
-> prove memory recovery -> reload under a new generation -> execute again. The
kernel is a deterministic low-rank (LoRA-like) transform so the adapter
materially changes computation and is verifiable against a CPU reference. No
physical multi-GPU, NVLink, RDMA, DPU, CXL, or distributed-accelerator behaviour
is claimed.

## Benchmarks

Measured throughputs are reported in `BENCHMARKS.md` (actual numbers only).

## Build / install / use

Requires CMake >= 3.24, MSVC (Windows-first), and CUDA 13.x for the accelerator
backend. On a Developer Command Prompt:

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release
    ctest --test-dir build -C Release
    cmake --install build --config Release --prefix <prefix>

A downstream consumer links via `find_package(AdapterFabric 1.0 CONFIG
REQUIRED)` and `adapter_fabric::adapter_fabric`. The CLI (`af_cli`) supports
`list, inspect, register, validate, load, activate, deactivate, compose,
migrate, invalidate, evict, pin, unpin, explain, snapshot, save, recover, serve,
worker, multiprocess, cuda, benchmark`.

## License

Apache License 2.0. Copyright 2026 Summon Software Labs. No telemetry transmission.
