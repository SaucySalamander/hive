# Task: Lineage Depth Cap on Re-queening

## Difficulty: Medium

## Background

The hive colony supports re-queening: when the current Queen's `perf_score`
drops below `HIVE_REQUEUE_THRESHOLD`, the highest-scoring worker is promoted to
Queen in `hive_queen_requeue_if_needed()` (`src/core/queen/queen.c`). Each
promotion increments `d->lineage_generation` in `hive_dynamics_t`.

There is currently no limit on how many times re-queening can occur. Under
adversarial workloads or a bad inference backend every worker could score
poorly and force repeated promotions, creating an arbitrarily deep lineage chain
with no stable Queen.

Relevant code in `hive_queen_requeue_if_needed()`:

```c
/* Bump lineage generation and propagate. */
d->lineage_generation++;
new_queen->traits.lineage_hash = d->lineage_generation;
```

And the bootstrap path (no workers present):

```c
d->lineage_generation++;
```

## Task

1. Add a constant `HIVE_MAX_LINEAGE_DEPTH` to
   `src/core/dynamics/dynamics.h` (or `src/core/scheduler/hive_config.h`).
   Default value: **8**. Protect it with `#ifndef`.

2. Add a `lineage_depth` field (type `uint32_t`) to `hive_dynamics_t` in
   `src/core/dynamics/dynamics.h`. This field tracks consecutive re-queening
   events since the last time a Queen completed a full successful pipeline cycle.

3. In `hive_queen_requeue_if_needed()`:
   - Before promoting, check `d->lineage_depth >= HIVE_MAX_LINEAGE_DEPTH`.
   - If the cap is reached:
     a. Log a `HIVE_LOG_WARN` message through the dynamics stats (set a new
        boolean field `lineage_cap_reached` on `hive_dynamics_stats_t`), OR
        simply increment `d->stats.active_alarms`.
     b. Reset the colony: call the bootstrap path (force-seed slot 0 as a fresh
        Queen with `perf_score = 80`, reset `d->lineage_depth = 0`,
        `d->lineage_generation` continues incrementing normally).
     c. Return `true` (a re-queening event did occur).
   - On a normal promotion, increment `d->lineage_depth`.

4. In `hive_scheduler_run()` (or `promote_queen_output()`) in
   `src/core/scheduler/scheduler.c`: when the Queen cell reaches
   `HIVE_AGENT_STAGE_DONE` and `promote_queen_output()` is called, reset
   `sched->dynamics->lineage_depth = 0` to indicate a successful cycle.

5. Expose `lineage_depth` in `hive_dynamics_stats_t.lineage_depth` so the GTK
   metrics panel can display it.

## Constraints

- Minimum changes: only `src/core/dynamics/dynamics.h`,
  `src/core/queen/queen.c`, `src/core/scheduler/scheduler.c`.
- Do not change any public scheduler or runtime API signatures.
- Build must pass with `make -j2`.

## Acceptance Criteria

- `make -j2` succeeds with zero new warnings.
- With `HIVE_MAX_LINEAGE_DEPTH=3`, after 3 consecutive failed re-queening
  events the colony is force-reset to a fresh Queen and `lineage_depth` returns
  to 0.
- After a successful Queen pipeline cycle, `lineage_depth` resets to 0.
- `lineage_depth` is visible in `dynamics->stats`.
