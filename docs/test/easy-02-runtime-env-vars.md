# Task: Runtime Env Var Coverage for Tuning Parameters

## Difficulty: Easy

## Background

The hive runtime currently reads only two environment variables at startup
(`HIVE_INFERENCE_BACKEND` and `HIVE_INFERENCE_CONFIG`), both in
`src/core/runtime.c`. All scheduler and quality thresholds are compile-time
constants from `src/core/scheduler/hive_config.h`:

| Constant | Default | Struct field set at init |
|---|---|---|
| `HIVE_SCHEDULER_MAX_CONCURRENT_WORKERS` | 8 | `sched->opts.max_concurrent_workers` |
| `HIVE_SCHEDULER_BACKOFF_MS_SUPPRESSED` | 100 | `sched->opts.backoff_ms_suppressed` |
| `HIVE_SCHEDULER_MIN_PERF_SCORE` | 10 | `sched->opts.min_perf_score` |
| `score_threshold` | 72 | `sched->opts.score_threshold` |
| `iteration_limit` | 3 | `sched->opts.iteration_limit` |
| `HIVE_VITALITY_MIN` | (in dynamics.h) | `d->cfg_vitality_min` |

The scheduler options struct `hive_scheduler_options_t` (in
`src/core/scheduler/scheduler.h`) already holds all of these at runtime, and
`hive_scheduler_init()` applies compile-time fallbacks when a field is zero. The
runtime initialises the scheduler in `hive_runtime_run()` in
`src/core/runtime.c`.

## Task

Add environment variable overrides for the six parameters above. The env vars
should be:

| Env var | Overrides |
|---|---|
| `HIVE_MAX_WORKERS` | `opts.max_concurrent_workers` |
| `HIVE_BACKOFF_MS` | `opts.backoff_ms_suppressed` |
| `HIVE_MIN_PERF` | `opts.min_perf_score` |
| `HIVE_SCORE_THRESHOLD` | `opts.score_threshold` |
| `HIVE_ITERATION_LIMIT` | `opts.iteration_limit` |
| `HIVE_VITALITY_MIN` | `d.cfg_vitality_min` (on `hive_dynamics_t`) |

Rules:

1. Parse each env var with `strtoul()`. Ignore the var if it is absent, empty,
   or non-numeric (i.e. endptr == value after the call).
2. Apply the parsed value only if it is within a sensible range:
   - `HIVE_MAX_WORKERS`: 1–64
   - `HIVE_BACKOFF_MS`: 1–60000
   - `HIVE_MIN_PERF`: 0–100
   - `HIVE_SCORE_THRESHOLD`: 0–100
   - `HIVE_ITERATION_LIMIT`: 1–20
   - `HIVE_VITALITY_MIN`: 0–100
3. Log each applied override at `HIVE_LOG_INFO` with the name and value.
4. Add a small helper function `parse_env_uint(const char *name, unsigned lo,
   unsigned hi, unsigned *out)` that handles steps 1–2 and returns `true` when
   a value was applied.

## Constraints

- All changes go into `src/core/runtime.c` only (no header changes needed).
- Do not change any existing function signatures.
- Build must pass with `make -j2`.

## Acceptance Criteria

- `make -j2` succeeds with zero new warnings.
- `HIVE_MAX_WORKERS=4 ./build/hive` uses 4 as `max_concurrent_workers`.
- `HIVE_MAX_WORKERS=999 ./build/hive` ignores the value (out of range).
- `HIVE_MAX_WORKERS=abc ./build/hive` ignores the value (non-numeric).
- Each applied override produces an `INFO` log line.
