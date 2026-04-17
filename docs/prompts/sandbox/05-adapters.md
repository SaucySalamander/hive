# Platform Adapters — Prompt

Short prompt

"For each target platform (Linux, macOS, Windows) and runtime option (seccomp, WASM, container), produce adapter designs and minimal integration skeletons." 

Expanded prompt

Context: Implementation will be incremental and platform-adaptable. The first target is Linux; macOS and Windows are optional later.

Tasks

1. For Linux/seccomp+namespaces: produce an adapter design with functions, required syscalls, example seccomp rules, integration points, and fallback behavior when features are unavailable.
2. For WASM/WASI: produce an adapter design using Wasmtime/Wasmer; propose mapping of syscall-like capabilities to host-provided APIs with capability tokens.
3. For container-based approach: propose an OCI runtime integration plan (runc or crun) and a minimal wrapper to start an agent inside a container image.
4. For Windows/macOS: sketch equivalent isolation strategies (Job Objects + AppContainer on Windows; macOS sandbox or hardened runtime; note gaps and priorities).

Deliverables

- Adapter interface definition for the sandbox module.
- Example skeleton code for Linux adapter (C header + small source showing how to setrlimit, unshare, apply seccomp filters, and exec/posix_spawn).
- Clear list of required privileges and kernel features.

Files to inspect

- `src/core/*`, existing process launch code (search for `fork`/`posix_spawn`), `Makefile`