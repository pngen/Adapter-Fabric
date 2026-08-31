// Adapter Fabric — real proofs (multiprocess, CUDA, benchmarks).
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <string>

namespace adapter_fabric {

// Real OS-process authority proof: a coordinator and at least two worker
// processes over framed TCP. Spawns the given coordinator/worker executables,
// drives the full sequence, kills one worker as an OS process, replays stale
// authority, and proves it is all rejected on the real transport. Returns 0 on
// success (nonzero = proof failed, with a reason written to stderr).
int run_multiprocess_proof(const std::string& coord_exe, const std::string& worker_exe, std::uint16_t port);

// Real RTX 5090 CUDA adapter-state proof: cold load -> allocate -> transfer ->
// kernel -> verify vs CPU -> warm reuse -> invalidate/evict -> release -> memory
// recovered -> reload under a new generation -> execute again. Returns 0 on success.
int run_cuda_proof();

// Benchmarks over useful operations. Writes measured ns/s values as lines of
// "name,ops,metric" to stdout. Returns 0 on success.
int run_benchmarks();

}  // namespace adapter_fabric
