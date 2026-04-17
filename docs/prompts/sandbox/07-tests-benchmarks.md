# Tests, Fuzzing & Benchmarks — Prompt

Short prompt

"Design a test plan and benchmark suite to validate sandbox correctness, security, and performance. Include concrete test cases, fuzz targets, and CI integration instructions." 

Expanded prompt

Context: Once a sandbox implementation exists we must verify it blocks attack vectors, behaves correctly under load, and keeps latency within acceptable bounds.

Tasks

1. Unit tests: API-level tests for `sandbox_init`, `sandbox_spawn_agent`, `sandbox_apply_policy`, and their error codes.
2. Integration tests: start a real agent binary under sandbox and test filesystem, network, and resource policies.
3. Fuzzing: define small fuzz targets for policy parsing and the agent-supervisor control channel (use AFL/LibFuzzer). Include example harnesses.
4. Security tests: try to exercise blocked syscalls and elevation attempts (simulate attempts to read `/etc/shadow`, create raw sockets, spawn privileged threads, etc.).
5. Performance benchmarks: microbenchmarks for agent startup time (sandboxed vs unsandboxed), throughput for 1000 short-lived agents, memory overhead.
6. CI integration: define a job matrix for Linux (and optionally macOS/Windows) that runs unit+integration+fuzzing (fuzzing can run periodically or on-demand).

Deliverables

- A prioritized test list with exact commands and expected outcomes.
- Example `Makefile` or shell commands to run tests and benchmarks.
- Suggested thresholds (e.g., startup overhead < 15ms average for trivial agent on developer hardware) — mark these as adjustable.

Files to inspect

- `Makefile`, any existing test harnesses under `tests/`