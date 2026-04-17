Title: PR-sized Tasks (Implementation Prompt)

Goal:
- Break the work into small, reviewable PRs. Each task below contains acceptance criteria and an effort estimate.

Task 1 — Add `agent_role_t` and lifecycle fields
- Files: `src/core/agent.c`, `src/core/agent.h`
- Changes: add enum, role field, age counter, getters/setters, unit tests.
- Acceptance: compiles, tests pass, role exposed via API.
- Effort: small (1–2 days).

Task 2 — Bounded message queue (safe first-pass)
- Files: `src/common/queue.h`, `src/common/queue.c`
- Changes: implement mutex+condvar bounded queue API (init,push,pop,close).
- Acceptance: thread-safe, unit-tested, no leaks (ASAN clean).
- Effort: small (0.5–1 day).

Task 3 — Signal subsystem (pheromones/waggle)
- Files: `src/core/signal.h`, `src/core/signal.c`
- Changes: publish/subscribe API, pooled signals, expiry mechanism.
- Acceptance: signals deliver to subscribers; expiry cleans memory; tests.
- Effort: medium (2–3 days).

Task 4 — Orchestrator integration
- Files: `src/core/orchestrator.c`, small edits to `src/core/planner.c`
- Changes: subscribe to signals, maintain pheromone map, implement quorum helper.
- Acceptance: orchestrator reacts to waggle adverts and triggers agent role transitions in test harness.
- Effort: medium (2–4 days).

Task 5 — Tests and microbenchmarks
- Files: `tests/dynamics/` (new folder) or `src/core/tester.c`
- Changes: unit tests for queue/signal/role logic; microbenchmark to measure signals/sec and round-trip latency.
- Acceptance: benchmarks run and included in PR; target metrics documented.
- Effort: small-to-medium (1–2 days).

Task 6 — Optional optimization (lock-free queue)
- Files: `src/common/queue_lockfree.c` (optional)
- Changes: implement and benchmark lock-free ring buffer for high-throughput paths.
- Acceptance: benchmark shows measurable improvement; maintains memory-safety (UBSAN/ASAN).
- Effort: medium (3–5 days).

Task 7 — CI sanitizers & benchmark baseline
- Files: CI config (e.g., `.github/workflows/ci.yml`), `tests/benchmarks/baseline.txt`
- Changes: add CI steps to build with ASAN/UBSAN, run unit tests, and run benchmark harness to record baseline metrics into `tests/benchmarks/baseline.txt`.
- Acceptance: CI runs sanitizers and unit tests; baseline file created and referenced in PRs that touch performance-sensitive code.
- Effort: small (0.5–1 day).

Task 8 — Spawn & lifecycle policy
- Files: `src/core/orchestrator.c`, `src/core/agent.c`, `src/core/planner.c`
- Changes: implement queen spawn/retire policy, add `agent` lifecycle fields, lazy role-state pool, spawn cooldowns, `target_role_fraction` config and unit tests (`test_spawn_policy`, `test_role_distribution`).
- Acceptance: tests verify spawn respects `min_workers`, `max_workers`, `spawn_rate_per_min`, and that role distribution converges to targets under simulated load.
- Effort: medium (2–3 days).

Task 9 — Drone experiments & proposal flow
- Files: `src/core/agent.c`, `src/core/signal.c`, `src/core/orchestrator.c`
- Changes: implement small drone runner, `SIGNAL_PROPOSAL` type, proposal publish/subscribe, and quorum-based adoption helper. Add `test_drone_proposal_flow`.
- Acceptance: proposals are published by drones, votes collected, and adoption only occurs when quorum helper returns true; tests simulate adoption and non-adoption.
- Effort: medium (2–3 days).

PR checklist (for each PR):
- Minimal, focused changes.
- Unit tests added/updated.
- Build passes on CI with sanitizers enabled.
- Short design note in PR description explaining trade-offs.

CI / benchmark notes:
- Add a CI job that builds with `CFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer'`, runs the test suite, and executes the benchmark harness to produce `tests/benchmarks/baseline.txt`.
- Include baseline numbers and a suggested regression threshold (e.g., allow up to 25% regression vs baseline for non-critical changes) in the PR description when performance is impacted.
