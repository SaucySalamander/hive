# Architecture Options & Trade-offs — Prompt

Short prompt

"Propose 3–4 viable sandbox architectures for hive agents, evaluate trade-offs (security, complexity, perf), and recommend the best first-stage architecture for incremental rollout." 

Expanded prompt

Context: Target host is Linux primarily; we prefer minimal dependencies and incremental safety improvements.

For each candidate architecture produce:
- Short description and diagram (process boundaries, data flows, trust boundaries).
- Security benefits and known limitations.
- Implementation complexity and required changes to the repo (files, build, runtime args).
- Performance characteristics and expected overheads.
- Failure modes and recovery strategy.

Candidate architectures to evaluate

1. Process isolation with kernel primitives: `unshare`/namespaces + `seccomp` + `setrlimit` + cgroups.
2. Embedded WASM runtime (WASI/Wasmtime) to run untrusted code in a memory-safe sandbox.
3. Dedicated agent worker processes inside lightweight containers (OCI runtimes) or firecracker microVMs.
4. Hybrid: run sensitive operations in a privileged helper; run model execution in WASM or confined process.

Deliverables

- Side-by-side trade-off matrix (security, perf, dev-effort, portability).
- Recommended first-stage architecture with a step-by-step PR plan and acceptance criteria.
- Migration path for future hardening (WASM or microVM as opt-in later).

Files to inspect

- `src/core/*`, `Makefile`, `build/`, `src/inference/*`