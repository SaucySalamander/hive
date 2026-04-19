# hive

`hive` (Harness for Interactive Verification & Execution) is a C23 repository skeleton for a safety-gated, hierarchical agent runtime built around a pure C state machine.

It includes structured JSON logging (implementation: `src/common/logging`), a pluggable inference adapter (implementation: `src/core/inference`), a tool registry with approval gates, a headless CLI using `argp` on GNU systems, an ncurses-based TUI, and an optional libuv/cJSON API server that stays disabled by default.

## Included

- Strict C23 build settings with `-Wall -Wextra -Werror -pedantic`
- MIT license
- GNU Make build system with auto-detected optional dependencies
- Linux-first implementation with portable feature detection
- Dummy inference adapter that echoes prompts so the project runs out of the box
- Dedicated agents for Orchestrator, Planner, Coder, Tester, Verifier, and Editor
- Evaluator/Reflexion loop that scores and critiques each output
- Tool registry stubs for `read_file`, `write_file`, `run_command`, `list_files`, `grep`, and `git_status`
- Structured logging to stderr and syslog

## Build

```sh
make build
```

The Makefile builds directly with `cc`/`gcc` and auto-enables optional features when their dependencies are available.

Optional features are enabled automatically when their dependencies are present, and you can force them on or off with `auto`, `1`, or `0`.

- `ncurses` enables the TUI when `HIVE_ENABLE_TUI=auto`.
- `libuv` + `cJSON` enable the API server when `HIVE_ENABLE_API=auto`.
- `argp` and `syslog` are detected automatically.

Examples:

```sh
make HIVE_ENABLE_TUI=0
make HIVE_ENABLE_API=1
make HIVE_ENABLE_SYSLOG=0
make run
```

Run the binary directly if you prefer:

```sh
./build/hive --prompt "Create a safe C23 agent harness skeleton"
```

## CLI usage

Run headless with the dummy inference adapter and approve tool execution explicitly:

```sh
./build/hive --prompt "Create a safe C23 agent harness skeleton" --yes
```

Useful flags:

- `--workspace <dir>`: set the workspace root used by tools
- `--rules <file>`: load extra project rules from a text file
- `--iterations <n>`: set the maximum refinement loops
- `--tui`: launch the interactive TUI instead of headless mode
- `--api`: start the optional HTTP API server if built in
- `--yes`: auto-approve tool execution in headless mode

## TUI usage

```sh
./build/hive --tui
```

In the TUI, every tool request is approved interactively before execution.

## Custom inference engine

Hive now routes completions through a small backend registry. The built-in mock backend is selected by default, and you can switch backends by setting `HIVE_INFERENCE_BACKEND` and `HIVE_INFERENCE_CONFIG` before launch.

The low-level public API lives in `src/core/inference/inference.h` and exposes `inf_create()`, `inf_complete_sync()`, `inf_complete_stream()`, `inf_list_models()`, and `inf_cancel()`.

If you want to keep using the legacy adapter wrapper, initialize a named backend with `hive_inference_adapter_init_named()` or fall back to the compatibility mock via `hive_inference_adapter_init_mock()`.

At a minimum, a backend must return an allocated UTF-8 string for each request and free it through the caller-owned `inf_free()` contract.

```c
static hive_status_t my_generate(void *state,
                                     const hive_inference_request_t *request,
                                     hive_inference_response_t *response)
{
    (void)state;
    (void)request;
    (void)response;
    return HIVE_STATUS_UNAVAILABLE;
}
```

Then wire that adapter into `src/core/runtime.c` or swap it in from your own application entry point.

## Project rules

The example rules file lives at `config/project.rules.example`. It is loaded automatically when present and can be copied or edited for your own workflow.
