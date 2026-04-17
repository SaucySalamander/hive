Title: Spawn & Lifecycle — Queen spawn rules, worker lifecycles, drones (Implementation Prompt)

Context:
- This prompt codifies spawn rules, lifecycle transitions, role→task mappings, drone experiment mechanics, retirement/eviction policies, and config defaults for the hive orchestration model. It complements the bee behavior notes in `docs/bees/dynamics.md` and the other dynamics prompts.

Goal:
- Provide a precise, implementable spec and small PR-sized tasks to add deterministic, testable spawn & lifecycle behavior to the orchestrator and agents. Include defaults, pseudocode, and tests so implementers can produce patches that meet CI/sanitizer requirements.

Constraints:
- Language: C (matching the repo). Keep per-agent memory bounded using lazy allocation and small pools. Make behaviors deterministic where possible for tests (use seeds/time windows).

Deliverables:
- `docs/architecture/hive-spawn-lifecycle.md` — short design doc (1–2 pages) summarizing policy, defaults, and trade-offs.
- PR #A — add `agent` lifecycle fields and lazy `role_state` pool.
- PR #B — orchestrator spawn/retire policy + config knobs and unit tests.
- PR #C — drone experiment pipeline + quorum adoption helper and tests.

Acceptance criteria:
- Build passes (`make`).
- Unit tests: `test_spawn_policy`, `test_role_distribution`, `test_retirement`, `test_drone_proposal_flow` run under ASAN/UBSAN and pass.
- Spawn respects caps, cooldowns, and quota targets under load tests.
- Drone proposals adopt only via quorum; no single-actor unilateral adoption.

Core policy (implementation-ready)

1) Spawn triggers and quotas
- Triggers (spawn when any condition true):
  - `active_workers < min_workers`
  - backlog length > `backlog_threshold`
  - avg_task_latency > `latency_threshold`
  - periodic rebalancing tick (e.g., every `rebalance_interval_ms`)
- Rate-limits & caps:
  - `spawn_rate_per_min` (max new workers per minute)
  - `spawn_cooldown_s` (cooldown between decision rounds)
  - `min_workers`, `max_workers` absolute bounds
  - `max_drones` small fraction of total (e.g., 2–5%)
- Target distribution: orchestrator keeps `target_role_fraction` and fills deficits up to spawn budget.

Suggested defaults (tunable):
- `min_workers = 5`
- `max_workers = 200`
- `spawn_rate_per_min = 5`
- `spawn_cooldown_s = 30`
- `target_role_fraction = { forager:0.40, builder:0.30, nurse:0.10, guard:0.10, cleaner:0.05, drone:0.05 }`

2) When to create (priority)
- Priority: satisfy immediate backlog (foragers/builders) → restore target distribution → spawn nurses/cleaners if onboarding/backlog grows → spawn guards if error-rate increases → spawn drones only for experiments.
- Respect resource budget; if resources are constrained, prefer retiring drones and cleaners first.

3) Worker lifetime & role handling
- Agent metadata (add to `agent_t`): `role`, `birth_ts_ns`, `age_ticks`, `last_role_change_ts_ns`, `role_state_ptr` (nullable), `perf_score`.
- Transition mechanisms:
  - Temporal (age/experience): new worker starts as `cleaner`/`onboard` for `initial_ticks` (default 3–10) then auto-promotes if skill thresholds met.
  - Event-driven (signals): orchestrator or pheromone/waggle signals can promote/demote agents based on shortages, backlog, or votes.
- Lazy role-state allocation: allocate per-role state only on first transition into the role and return to a small pool on exit or after `role_state_idle_timeout_ms`.

4) Role → Task mapping (practical mapping from user notes)
- `cleaners` → handle tech-debt, small bug fixes, quick low-risk tasks; reduce task complexity for builders.
- `nurses` → mentor/validate `cleaners`, review their outputs, generate training artifacts, annotate examples into the knowledge graph.
- `builders & grocers` → main feature work and integration; produce artifacts (builds, PRs, merged changes).
- `guards` → review/test: run static analysis, contract tests, and runtime invariants; flag rework if checks fail.
- `foragers` → knowledge expansion: fetch and ingest external docs, produce embeddings, update internal knowledge graph; tag provenance and rate-limit external calls.
- `drones` → experimental agents using mutated prompts/policies to propose improvements; run on small sample workloads.

5) Drones & evolutionary pressure
- Drone population small and ecologically managed: `max_drones` default 2–5%.
- Drones run experimental policies; successful variants generate `proposal` signals describing the change and measured delta.
- Proposal adoption: publish `SIGNAL_PROPOSAL`; collect votes via quorum window (see `QUORUM_WINDOW_MS`); adopt only if threshold reached.
- Queen coordinates resource allocation but does not unilaterally change global policies — adoption requires quorum

6) Queen role & non-central decision model
- Queen = policy steward & user-facing interface; emits recommendations and enforces resource budgets.
- Decision-making is decentralized: queen proposes actions via signals; agents decide/adopt via quorum.

7) Retirement / eviction rules
- Evict agents by: age (`max_age_ticks`), low `perf_score`, or under resource pressure.
- Eviction order under pressure: drones → cleaners → oldest foragers → others.

8) Metrics, logging and determinism
- Log role transitions with `agent_id`, `prev_role`, `new_role`, `reason`, `ts`.
- Use seeded randomness and fixed time windows in tests (e.g., `--benchmark-seed`) for reproducibility.

9) Pseudocode (spawn decision)

```c
void orchestrator_tick(orchestrator_t *o) {
    if (now_ns() < o->last_spawn_ts_ns + spawn_cooldown_s*1e9) return;
    calc_active_workers(o);
    calc_backlog(o);
    if (active_workers < min_workers || backlog > backlog_threshold || avg_latency > latency_threshold) {
        role = choose_role_by_deficit(o->target_role_fraction);
        spawn_worker(role);
        o->last_spawn_ts_ns = now_ns();
    }
}
```

10) Pseudocode (promotion rule)

```c
void on_task_complete(agent_t *a, task_t *t) {
    a->age_ticks++;
    if (a->age_ticks < initial_ticks) return; // remain cleaner
    score = compute_skill_score(a);
    if (score >= nurse_threshold) agent_set_role(a, ROLE_NURSE);
    else if (backlog_for_builders > X && score >= builder_min) agent_set_role(a, ROLE_BUILDER);
}
```

11) Files to edit / tests to add
- `src/core/agent.h` / `src/core/agent.c` — role fields, lazy role-state, logging hooks
- `src/core/orchestrator.c` — spawn/retire policy, config knobs
- `src/core/planner.c` — prefer role-aware planning hooks
- `src/core/signal.c` / `src/core/signal.h` — proposal signal type and TTL semantics
- Tests: `tests/dynamics/test_spawn_policy.c`, `test_role_distribution.c`, `test_retirement.c`, `test_drone_proposal_flow.c`

12) Benchmarks
- `bench_spawn_rate`: simulate churn for N agents, measure spawn/retire throughput and latency.

Clarifying questions for implementer
- Which concurrency model for MVP: single-threaded event-loop or small-thread-pool?
- Acceptable spawn/retire latency targets (ms) and budget constraints for resource gating?

Notes:
- This prompt is intentionally prescriptive: implementers should follow defaults but expose the knobs in config so maintainers can tune them with benchmarks.
