# hive

`hive` (Human Interactive Verification Engine) is a C23 repository skeleton for a safety-gated, hierarchical agent runtime built around a pure C state machine.

It includes structured JSON logging, a pluggable inference adapter, a tool registry with approval gates, a headless CLI using `argp` on GNU systems, an ncurses-based TUI, and an optional libuv/cJSON API server that stays disabled by default.

## Included

- Strict C23 build settings with `-Wall -Wextra -Werror -pedantic`
- MIT license
- CMake 3.22+ build system with Ninja support
- Linux-first implementation with macOS/Windows-friendly CMake feature detection
- Dummy inference adapter that echoes prompts so the project runs out of the box
- Dedicated agents for Orchestrator, Planner, Coder, Tester, Verifier, and Editor
- Evaluator/Reflexion loop that scores and critiques each output
- Tool registry stubs for `read_file`, `write_file`, `run_command`, `list_files`, `grep`, and `git_status`
- Structured logging to stderr and syslog

## Build

```sh
make build
```

If you want the direct CMake invocation instead:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Optional features are enabled automatically when their dependencies are present.

- `ncurses` enables the TUI when `HIVE_ENABLE_TUI=ON`.
- `libuv` + `cJSON` enable the API server when `HIVE_ENABLE_API=ON`.

Example with the API server enabled:

```sh
cmake -S . -B build -G Ninja -DHIVE_ENABLE_API=ON
cmake --build build
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

Replace the mock adapter by implementing `charness_inference_adapter_vtable_t` and initializing a custom adapter with `charness_inference_adapter_init_custom()`.

At a minimum, your engine must implement a `generate` callback that returns an allocated UTF-8 string for each request.

```c
static charness_status_t my_generate(void *state,
                                     const charness_inference_request_t *request,
                                     charness_inference_response_t *response)
{
    (void)state;
    (void)request;
    (void)response;
    return CHARNESS_STATUS_UNAVAILABLE;
}
```

Then wire that adapter into `src/core/runtime.c` or swap it in from your own application entry point.

## Project rules

The example rules file lives at `config/project.rules.example`. It is loaded automatically when present and can be copied or edited for your own workflow.
