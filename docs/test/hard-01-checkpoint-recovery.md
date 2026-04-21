# Task: Session Checkpoint and Crash Recovery

## Difficulty: Hard

## Background

The hive scheduler runs a multi-stage pipeline per worker cell
(ORCHESTRATE → PLAN → CODE → TEST → VERIFY → EDIT → EVALUATE → DONE).
Each stage calls an inference backend and accumulates output in
`hive_agent_context_t` (defined in `src/core/scheduler/scheduler.h`):

```c
typedef struct hive_agent_context {
    hive_agent_stage_t  current_stage;
    unsigned            iteration;
    unsigned            cycle_count;
    char               *orchestrator_brief;
    char               *plan;
    char               *code;
    char               *tests;
    char               *verification;
    char               *edit;
    char               *final_output;
    char               *last_critique;
    hive_score_t        last_score;
    bool                in_use;
} hive_agent_context_t;
```

A crash (SIGTERM, OOM kill, power loss) mid-cycle discards all accumulated
output and the next run starts from ORCHESTRATE. For long workloads this wastes
significant inference budget.

The global session in `hive_runtime_t.session` (`src/core/runtime.h`) mirrors
the Queen cell's context after each completed cycle; it is also lost on crash.

## Task

Implement a file-based checkpoint/restore mechanism for the Queen cell's
pipeline context. The goal is that after a crash the scheduler can detect the
checkpoint and skip already-completed stages.

### Checkpoint format

Use a simple line-delimited text format (no external dependencies):

```
HIVE_CHECKPOINT_V1
stage=<stage_name>
iteration=<n>
cycle_count=<n>
score_correctness=<n>
score_security=<n>
score_style=<n>
score_test_coverage=<n>
score_overall=<n>
---orchestrator_brief---
<base64-encoded content>
---plan---
<base64-encoded content>
---code---
<base64-encoded content>
---tests---
<base64-encoded content>
---verification---
<base64-encoded content>
---edit---
<base64-encoded content>
---final_output---
<base64-encoded content>
---last_critique---
<base64-encoded content>
---end---
```

Base64-encode each field value so newlines inside stage outputs are safe.
You may use a simple standard base64 implementation (write a minimal
`base64_encode` / `base64_decode` as static helpers; no external library).

### New files

Create `src/core/checkpoint.h` and `src/core/checkpoint.c`.

```c
/* src/core/checkpoint.h */

#ifndef HIVE_CORE_CHECKPOINT_H
#define HIVE_CORE_CHECKPOINT_H

#include "core/scheduler/scheduler.h"
#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write a checkpoint for the Queen cell's context to @path.
 * Atomic: writes to a temp file then renames.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_checkpoint_save(const char           *path,
                                   const hive_agent_context_t *ctx);

/**
 * Load a checkpoint from @path into @ctx_out.
 * Allocates heap strings; caller must call hive_checkpoint_free() when done.
 * @return HIVE_STATUS_OK on success; HIVE_STATUS_NOT_FOUND if file absent.
 */
hive_status_t hive_checkpoint_load(const char           *path,
                                   hive_agent_context_t *ctx_out);

/**
 * Free heap strings inside @ctx that were allocated by hive_checkpoint_load().
 * Does not free @ctx itself.
 */
void hive_checkpoint_free(hive_agent_context_t *ctx);

/**
 * Delete the checkpoint file at @path (called on successful pipeline completion).
 */
void hive_checkpoint_delete(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_CORE_CHECKPOINT_H */
```

### Integration into scheduler

In `src/core/scheduler/scheduler.c`, in `hive_scheduler_run()`:

1. **On startup**: derive the checkpoint path as
   `<runtime->session.workspace_root>/.hive_checkpoint`. Call
   `hive_checkpoint_load()`. If it returns `HIVE_STATUS_OK`:
   - Copy loaded fields into `sched->contexts[sched->dynamics->queen_idx]`.
   - Log at `HIVE_LOG_INFO` that a checkpoint was found and which stage is
     being resumed.
2. **After each successful stage dispatch for the Queen cell**: call
   `hive_checkpoint_save()` with the Queen's context. Failures are logged at
   `HIVE_LOG_WARN` but do not abort the run.
3. **After `promote_queen_output()`**: call `hive_checkpoint_delete()` to
   remove the checkpoint once the cycle completes cleanly.

### Constraints

- Base64 helpers are static functions inside `checkpoint.c`; no external lib.
- Atomic write: write to `<path>.tmp` then `rename()` to `<path>`.
- `hive_checkpoint_save` / `hive_checkpoint_load` must be async-signal-safe
  enough to survive an abrupt process kill between the write and rename
  (the temp file is simply ignored on next load if rename did not complete).
- Do not hold any locks during file I/O.
- Add `src/core/checkpoint.c` to the Makefile's `SRCS` variable.
- Build must pass with `make -j2`.

## Acceptance Criteria

- `make -j2` succeeds with zero new warnings.
- Running the hive and killing it after the PLAN stage produces a
  `.hive_checkpoint` file in the workspace root containing `stage=code`.
- Re-running the hive loads the checkpoint, logs the resume message, and skips
  ORCHESTRATE and PLAN (starts from CODE).
- A completed run deletes the checkpoint file.
- A corrupted checkpoint file (truncated) is silently ignored and a fresh run
  starts from ORCHESTRATE.
