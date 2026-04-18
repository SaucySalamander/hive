# CopilotCLI adapter — Implementation prompt

Context
- Some deployments will prefer a CLI-based client (copilotcli) or third-party CLI that streams tokens to stdout. A process-based adapter provides a pragmatic, dependency-light integration path.

Goal
- Implement a process-based adapter that spawns the `copilotcli` binary, writes requests to its stdin, reads streaming output from stdout, and maps lifecycle/cancellation to `inf_cancel()`.

Constraints
- Robustness: use `posix_spawn`/`fork+exec` with non-blocking pipes and a reader thread or event loop to deliver stream tokens.
- Cancellation: send a graceful interrupt (SIGINT) to the child process and fall back to SIGTERM after a timeout.
- Tests: provide a fake CLI binary used by CI tests to reliably exercise stream/cancel semantics.

Deliverables (PR-sized)
- `src/inference/adapters/copilotcli/copilotcli.c` and `copilotcli.h` — process adapter implementation.
- `tests/inference/copilotcli_contract.c` — contract tests using a test double CLI (a small C program that prints tokens and reacts to SIGINT).
- `docs/inference/adapters/copilotcli.md` — README describing config and model mapping.

Acceptance criteria
- `inf_complete_stream()` invokes `inf_stream_cb_t` with token fragments produced by the CLI.
- `inf_cancel()` terminates the in-flight CLI process and the contract test observes `INF_ERR_CANCELLED`.
- `inf_list_models()` returns models by running `copilotcli --list-models` or via config mapping.

Config JSON example
```json
{
  "cmd": "/usr/local/bin/copilotcli",
  "args": ["--no-color"],
  "env": { "CO-PILOT_KEY": "ENV:CO-PILOT_KEY" },
  "model_map": { "hive:default": "gpt-4o-mini" }
}
```

Starter snippet (concept)
```c
static int copilotcli_complete_stream(void *handle, const char *req_json,
                                     inf_stream_cb_t cb, void *user, int *cancel_out) {
  int inpipe[2], outpipe[2];
  pid_t pid;
  pipe(inpipe); pipe(outpipe);
  pid = fork();
  if (pid == 0) {
    dup2(inpipe[0], STDIN_FILENO);
    dup2(outpipe[1], STDOUT_FILENO);
    execv("/usr/local/bin/copilotcli", (char *const[]){"copilotcli", NULL});
    _exit(127);
  }
  /* parent: close unused ends, write req_json to child's stdin, spawn a reader thread
     that reads stdout and calls cb(token, len, user). Save pid -> cancel_token mapping. */
  return INF_OK;
}
```

Tests
- Implement `tests/inference/copilotcli_fake.c`: a tiny program that reads a JSON request and writes a sequence of newline-delimited token frames, and exits on EOF or when receiving SIGINT.
