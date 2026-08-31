# Adapter Fabric — Examples

Copyright 2026 Summon Software Labs. Apache-2.0.

After `cmake --build build --config Release`, the examples live in
`build/examples/Release/`.

| Example | Demonstrates |
|---------|--------------|
| `ex_register` | basic registration, validation (compatible), and structured incompatibility rejection on a wrong model revision |
| `ex_compat` | deterministic, explainable compatibility evidence (text + JSON) |
| `ex_load_activate` | load -> ready -> activate -> deactivate -> evict with a local worker |
| `ex_compose` | immutable composition, deterministic digest, total memory cost |
| `ex_migrate` | governed migration FSM (destination must be proven before authority transfer) |
| `ex_invalidate` | invalidation after a base-model revision change clears authority |
| `ex_persist` | persistence/recovery restores canonical metadata but never authority |
| `ex_restart` | worker restart rejects a stale-boot fence |
| `ex_cuda` | real RTX 5090 CUDA adapter execution/reuse proof |

Run, for example:
```
build\examples\Release\ex_register.exe
build\examples\Release\ex_cuda.exe
```

## CLI

`af_cli` operates a local single-node fabric for metadata and load/activate
commands, and exposes `serve`, `worker`, `multiprocess`, `cuda`, and
`benchmark` for the distributed and accelerator paths.
```
build\Release\af_cli.exe register my-lora
build\Release\af_cli.exe list
build\Release\af_cli.exe cuda
build\Release\af_cli.exe benchmark
build\Release\af_cli.exe serve --port 24000
build\Release\af_cli.exe worker --port 24000 --name A --seed 1
```

`af_coordinator` and `af_worker` are the real distributed processes driven by
the multiprocess proof.
