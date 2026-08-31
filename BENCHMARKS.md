# Adapter Fabric — Benchmarks

Copyright 2026 Summon Software Labs. Apache-2.0.

Measured on the RTX 5090 host (MSVC 19.44, CUDA 13.1, Release x64). Run
`build\benchmarks\Release\af_bench.exe` or `af_cli benchmark`. Reported are
actual single-run values.

| Operation | Calls | ns/op |
|-----------|-------|-------|
| identity parse/round-trip | 200 000 | ~154 |
| adapter register | 20 000 | ~810 |
| compatibility evaluate | 50 000 | ~1110 |
| composition construct | 20 000 | ~1290 |
| residency create + lookup | 200 000 | ~1480 |
| persistence save + recover | 5 000 | ~2695 |

These are microbenchmarks of the canonical, in-process paths. The real
multiprocess authority proof and the CUDA adapter proof are covered in the
test suite (`test_multiprocess`, `test_cuda`) and produce their own
end-to-end pass/fail evidence.

## CUDA cold / warm / transfer

`test_cuda` (the RTX 5090 proof) validates cold load, kernel execution, warm
reuse without redundant reload, eviction + release, memory recovery, and reload
under a new generation against an exact CPU reference. Timing-sensitive
per-operation CUDA numbers are intentionally not hyper-scoped; the proof's
value is correctness and recovery on real accelerator state.
