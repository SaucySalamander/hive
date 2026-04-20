# Hive Dynamics (Simulation Kernel)

Short summary
--

`hive_dynamics` is the in-memory simulation of the colony: it owns the fixed-size agent array, lifecycle templates, running statistics, and the per-tick progression logic. Implementation: [src/core/dynamics/dynamics.h](src/core/dynamics/dynamics.h) and [src/core/dynamics/dynamics.c](src/core/dynamics/dynamics.c).

Responsibilities
--

- **Lifecycle registry**: `hive_lifecycle_init_registry()` registers built-in templates (worker, queen, drone) and exposes lookup/registration APIs.
- **Agent array management**: `hive_dynamics_init()` initializes `d->agents[]` (queen at slot 0 by default) and seeds runtime thresholds.
- **Tick processing**: `hive_dynamics_tick()` orchestrates the per-tick sequence: Queen pheromone emission, re-queening, agent ageing/transitions, performance drift, demand-driven spawning, and stats recomputation.
- **Statistics**: `hive_dynamics_recompute_stats()` builds `hive_dynamics_stats_t` for UI and diagnostics.

Runtime flow (per tick)
--

1. `hive_queen_emit_pheromone()` — update `vitality_checksum` and per-agent `vitality_seen`/`conditioned_ok`.
2. `hive_queen_requeue_if_needed()` — promote best worker when the Queen is weak.
3. Per-agent updates — age, lifecycle template transitions via `hive_role_for_age()`, stochastic transitions for legacy templates, and perf_score drift.
4. `hive_queen_regulate_population()` — spawn workers to meet backlog-driven demand.
5. `hive_dynamics_recompute_stats()` — refresh counters for UI and telemetry.

Mermaid sketch (tick order)
```mermaid
flowchart TD
  Tick["hive_dynamics_tick()"] --> Emit[hive_queen_emit_pheromone]
  Emit --> Requeue[hive_queen_requeue_if_needed]
  Requeue --> Agents[Per-agent updates (age, role, perf)]
  Agents --> Regulate[hive_queen_regulate_population]
  Regulate --> Recompute[hive_dynamics_recompute_stats]
```

Key types & limits
--

- `HIVE_DYNAMICS_MAX_AGENTS` (256) — fixed array capacity in `hive_dynamics_t`.
- `hive_lifecycle_template_t`, `hive_agent_cell_t`, and `hive_dynamics_stats_t` — see [src/core/dynamics/dynamics.h](src/core/dynamics/dynamics.h).

Integration points
--

- The dynamics module calls the Queen functions (`src/core/queen/*`) and is driven externally by whoever advances ticks (TUI, tests, or a supervisor). The demand buffer (`d->demand_buffer_depth`) is written by external producers (UI or runtime components) to indicate backlog.

Open questions / extension points
--

- Source of `d->demand_buffer_depth`: document producers and expected units (tasks/ticks).
- Lifecycle templates: adding configurable templates at runtime is supported via the registry; recommend documenting a small set of example templates and tooling for editing them.

---

## Option 3 — New Fields & Invariants

### `hive_agent_cell_t` additions

| Field | Type | Invariant |
|---|---|---|
| `bound_agent` | `struct hive_agent *` | Non-NULL for every active cell except DRONE/EMPTY; freed by `retire_cell()` or `hive_scheduler_deinit()`. |
| `binding_generation` | `uint32_t` | Starts at 1 on spawn; incremented by Queen rebind on re-queening. |

### `hive_groom_packet_t` additions

| Field | Type | Meaning |
|---|---|---|
| `stage_latency_ns` | `uint64_t` | Wall-clock time for the dispatched stage (set by scheduler). |
| `confidence_score` | `uint8_t` | Derived from `ctx->last_score.overall` (0–100). |
| `anomaly_flags` | `uint8_t` | Bit 0 = low perf; bit 1 = stage error. |

### `hive_dynamics_stats_t` additions

| Field | Type | Meaning |
|---|---|---|
| `active_bound_workers` | `unsigned` | Cells with `bound_agent != NULL`. |
| `suppressed_workers` | `unsigned` | Cells with `conditioned_ok == false`. |
| `average_pheromone_latency_ns` | `uint64_t` | Rolling average stage latency (set by scheduler). |
| `requeen_events_this_run` | `unsigned` | Times `queen_idx` changed since `hive_scheduler_init()`. |

### `hive_dynamics_deinit()` — new function

Frees all `bound_agent` heap pointers still set on cells (e.g. if the
caller destroys dynamics before the scheduler), then `memset`s the struct
to zero.  The scheduler always frees bound_agent before returning to
dynamics, so this serves only as a safety-net fallback.
