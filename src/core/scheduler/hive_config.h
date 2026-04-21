/*
 * OPTION 3 IMPLEMENTATION — WORKER-CELL MAPPING
 * hive_config.h — compile-time defaults for the scheduler / dynamics binding.
 *
 * Every value can be overridden before the first inclusion of this header or
 * via -DHIVE_SCHEDULER_MAX_CONCURRENT_WORKERS=4 on the compiler command line.
 */

#ifndef HIVE_CORE_SCHEDULER_CONFIG_H
#define HIVE_CORE_SCHEDULER_CONFIG_H

/* ----------------------------------------------------------------
 * Scheduler tuning
 * ---------------------------------------------------------------- */

/** Maximum worker cells dispatched per scheduler iteration (v1 is single-threaded). */
#ifndef HIVE_SCHEDULER_MAX_CONCURRENT_WORKERS
#  define HIVE_SCHEDULER_MAX_CONCURRENT_WORKERS 8U
#endif

/**
 * Back-off in milliseconds when the colony is Queenless or every active cell
 * has conditioned_ok == false.  Prevents busy-spin on a suppressed colony.
 */
#ifndef HIVE_SCHEDULER_BACKOFF_MS_SUPPRESSED
#  define HIVE_SCHEDULER_BACKOFF_MS_SUPPRESSED 100U
#endif

/**
 * Minimum perf_score required for a cell to be considered eligible for
 * dispatch.  Cells below this threshold are counted as suppressed.
 */
#ifndef HIVE_SCHEDULER_MIN_PERF_SCORE
#  define HIVE_SCHEDULER_MIN_PERF_SCORE 10U
#endif

/**
 * Number of consecutive alarm packets that trigger permanent worker quarantine.
 * Once reached the worker's conditioned_ok is cleared and perf_score zeroed.
 */
#ifndef HIVE_QUARANTINE_ALARM_THRESHOLD
#  define HIVE_QUARANTINE_ALARM_THRESHOLD 3U
#endif

/**
 * Wall-clock nanoseconds after which a stage that returned HIVE_STATUS_OK is
 * still treated as a failure (HIVE_STATUS_IO_ERROR) for grooming purposes.
 * Default: 120 seconds.
 */
#ifndef HIVE_STAGE_TIMEOUT_NS
#  define HIVE_STAGE_TIMEOUT_NS (120ULL * 1000000000ULL)  /* 120 seconds */
#endif

/* ----------------------------------------------------------------
 * Binding mode constants
 * ---------------------------------------------------------------- */

/** One agent descriptor bound permanently to exactly one cell (v1 default). */
#define HIVE_BINDING_MODE_1_TO_1 0U

/**
 * Stub — reserved for a future N:1 sharing mode where multiple cells share a
 * single agent descriptor instance.  Not implemented in v1.
 */
#define HIVE_BINDING_MODE_N_TO_1 1U

/* ----------------------------------------------------------------
 * Legacy scheduler escape hatch
 *
 * Pass -DHIVE_LEGACY_SCHEDULER=1 to revert hive_runtime_run() to the
 * pre-Option-3 hive_state_machine_run() path.  Available for one release
 * cycle to allow a smooth rollout.
 * ---------------------------------------------------------------- */
#ifndef HIVE_LEGACY_SCHEDULER
#  define HIVE_LEGACY_SCHEDULER 0
#endif

/* ----------------------------------------------------------------
 * Single-threaded build flag
 *
 * Pass -DHIVE_SINGLE_THREADED=1 to compile out pthread_rwlock usage.
 * Safe when the scheduler and all other dynamics consumers are guaranteed to
 * execute on the same OS thread (e.g. embedded builds).
 * ---------------------------------------------------------------- */
#ifndef HIVE_SINGLE_THREADED
#  define HIVE_SINGLE_THREADED 0
#endif

#endif /* HIVE_CORE_SCHEDULER_CONFIG_H */
