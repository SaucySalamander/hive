# Starter Code Request — Prompt

Short prompt

"Produce a minimal, memory-safe starter patch (apply_patch-style) that adds a `sandbox` module with a Linux-first implementation using `seccomp` + `setrlimit` + `posix_spawn` to isolate agents. Keep it opt-in and testable." 

Expanded prompt

Context: The repo builds with `Makefile`. Keep the change small and reviewable. The goal is an initial PR that demonstrates end-to-end isolation for a simple agent process.

Requirements for the requested patch

- Files: `src/core/sandbox.h`, `src/core/sandbox.c`, `tests/sandbox_basic.c` (unit test harness).
- API (minimal):
  - `int sandbox_init(void);`
  - `int sandbox_spawn_agent(const char *path, char *const argv[], const char *policy_json, pid_t *out_pid);`
  - `int sandbox_apply_policy(pid_t pid, const char *policy_json);`
- Implementation notes:
  - Use `posix_spawn` or `fork+exec` with `setrlimit` (RLIMIT_AS, RLIMIT_NOFILE) and `prctl(PR_SET_NO_NEW_PRIVS, 1)`.
  - Apply a small seccomp filter that blocks `open_by_handle_at`, `ptrace`, `clone` with CLONE_NEWUSER if possible (example minimal allowlist approach).
  - Avoid dynamic allocations without bounds checks; prefer stack buffers and clear return codes.
  - Keep feature-flag compile guards so the sandbox can be disabled at compile or runtime.
- Tests:
  - A minimal test that launches an agent which attempts a forbidden syscall (e.g., raw socket) and asserts it is blocked.
  - A second test asserting normal agent behavior works when sandbox disabled.

Deliverable format

- Provide an `apply_patch` style diff or an exact file contents for the three files above.
- Add concise `Makefile` change instructions to compile the tests.

Safety notes (to the implementer/LLM)

- Emphasize memory-safety in C: explicit bounds checks, no unchecked snprintf, and clear ownership of memory.
- Keep the first implementation as a demonstrator — correctness over completeness.

Files to inspect

- Existing process-launch code and tests under `src/` and `tests/` (if tests directory exists).