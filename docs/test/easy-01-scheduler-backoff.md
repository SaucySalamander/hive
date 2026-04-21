# Task: Exponential Backoff on Suppressed Colony

## Difficulty: Easy

## Background

The hive scheduler in `src/core/scheduler/scheduler.c` contains a helper:

```c
static void scheduler_backoff(unsigned ms)
{
    if (ms == 0U) return;
    struct timespec ts;
    ts.tv_sec  = (time_t)(ms / 1000U);
    ts.tv_nsec = (long)((ms % 1000U) * 1000000L);
    nanosleep(&ts, NULL);
}
```

This function is called in `hive_scheduler_run()` when the entire colony is
suppressed (all workers fail the `conditioned_ok` pheromone gate) or when no
eligible workers exist. The call looks like:

```c
scheduler_backoff(sched->opts.backoff_ms_suppressed);
```

`sched->opts.backoff_ms_suppressed` is initialized to the compile-time default
`HIVE_SCHEDULER_BACKOFF_MS_SUPPRESSED` (100 ms) if not overridden. The current
code sleeps a flat 100 ms on every suppressed iteration, regardless of how long
suppression has lasted.

## Problem

Under sustained suppression the scheduler busy-loops with a fixed 100 ms delay.
This wastes CPU time in the common transient case (a single bad inference tick)
and does not signal escalating stress to operators.

## Task

Replace the flat sleep with an exponential backoff with jitter. Specifically:

1. Add a `suppressed_ticks` counter inside `hive_scheduler_run()` (local variable,
   reset to 0 when eligible workers are found).
2. Compute the sleep duration each suppressed tick as:

   ```
   delay_ms = base_ms * (2 ^ min(suppressed_ticks, 5))
   ```

   where `base_ms = sched->opts.backoff_ms_suppressed` (default 100 ms), so
   the cap is `base_ms * 32` (3 200 ms maximum).

3. Add ±20 % uniform jitter using `rand()` (seed once with `time(NULL)` at the
   start of `hive_scheduler_run`).

4. Keep the existing `scheduler_backoff(unsigned ms)` helper; just change the
   value passed into it.

5. Log (at `HIVE_LOG_WARN`) when suppression persists beyond 3 consecutive ticks,
   including the current `suppressed_ticks` count and computed delay.

## Constraints

- Do not change any public header signatures.
- Do not add new files; all changes go into `src/core/scheduler/scheduler.c`.
- The backoff must reset to `base_ms` immediately once at least one eligible
  worker is dispatched.
- Build must pass with `make -j2`.

## Acceptance Criteria

- `make -j2` succeeds with zero new warnings.
- With `HIVE_SCHEDULER_BACKOFF_MS_SUPPRESSED=50` and all workers suppressed,
  consecutive sleep durations increase: 50, 100, 200, 400, 800, 1600, 3200, 3200, ...
- A `HIVE_LOG_WARN` log entry appears after 3 consecutive suppressed ticks.
- Sleep resets to base on the next tick where a worker is dispatched.
