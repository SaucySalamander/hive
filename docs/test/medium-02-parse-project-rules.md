# Task: Parse and Enforce Project Rules File

## Difficulty: Medium

## Background

The hive runtime loads a plain-text project rules file (e.g.
`config/project.rules.example`) in `load_project_rules()` in
`src/core/runtime.c`. The content is stored in `runtime->session.project_rules`
and injected verbatim as context into agent prompts. It is never parsed or
enforced programmatically.

The current `config/project.rules.example` contains lines like:

```
workspace_root = "."
safety_policy = "approval_required_for_every_tool"
iteration_budget = 3
focus = "correctness security style test_coverage"
inference_mode = "mock_echo_until_custom_adapter_is_plugged_in"
```

The scheduler options struct `hive_scheduler_options_t` already holds
`iteration_limit` and `score_threshold`. The scheduler is initialized in
`hive_runtime_run()` in `src/core/runtime.c`.

## Task

Add a parser that extracts key-value pairs from the project rules text and
applies the following fields to the runtime before the scheduler starts:

| Rules key | Maps to |
|---|---|
| `iteration_budget` | `hive_scheduler_options_t.iteration_limit` |
| `score_threshold` | `hive_scheduler_options_t.score_threshold` |
| `vitality_min` | `hive_dynamics_t.cfg_vitality_min` |
| `max_workers` | `hive_scheduler_options_t.max_concurrent_workers` |

### Parser Requirements

1. Write a static function `parse_rules_uint(const char *rules_text,
   const char *key, unsigned lo, unsigned hi, unsigned *out)` in
   `src/core/runtime.c` that:
   - Searches `rules_text` for a line matching `key = <value>` (leading and
     trailing whitespace allowed around `=`).
   - Parses `<value>` as an unsigned decimal integer.
   - Applies it only if it is in `[lo, hi]`.
   - Returns `true` if a valid value was found and written to `*out`.

2. After `load_project_rules()` returns in `hive_runtime_run()`, call
   `parse_rules_uint` for each of the four keys above. Apply results to the
   local `hive_scheduler_options_t` struct **before** calling
   `hive_scheduler_init()`.

3. Log each applied override at `HIVE_LOG_INFO` with key and value.

4. Values parsed from env vars (from the env-var coverage task) should take
   precedence over values from the rules file (apply rules first, then env vars
   so env vars win).

### Parsing Rules

- Lines starting with `#` are comments; skip them.
- Match is case-sensitive.
- Only the first occurrence of each key is used.
- If the value is quoted (`"3"`) strip the quotes before parsing.

## Constraints

- All changes go into `src/core/runtime.c` only.
- Do not change any existing public headers or function signatures.
- Build must pass with `make -j2`.

## Acceptance Criteria

- `make -j2` succeeds with zero new warnings.
- A rules file containing `iteration_budget = 5` causes the scheduler to use
  `iteration_limit = 5`.
- An env var `HIVE_ITERATION_LIMIT=2` overrides the rules-file value.
- A rules file containing `score_threshold = 80` causes `opts.score_threshold = 80`.
- Lines with unrecognised keys are silently ignored.
- Comment lines (`# ...`) are silently ignored.
