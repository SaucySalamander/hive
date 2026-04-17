# Inference backend abstraction — Overview

Context
- Hive defines the agent construct and high-level logic inside the codebase. Actual LLM/Inference calls should be pluggable so the same agent code can run against different backends (OpenAI, Anthropic, local llama/gpt4all, internal hosted endpoints).

Goal
- Provide a small, well-defined abstraction layer between Hive agents and inference providers so the runtime can select and interchange backends without changing agent logic.

Constraints
- C-first (project uses C) but language-agnostic design so adapters may be implemented in C, D, or as an external process.
- Minimal dependencies: adapters may depend on libcurl, grpc, or vendor SDKs but core API must be lightweight.
- Memory-safety: clearly documented ownership rules and explicit free functions.
- Support sync and streaming completions, model listing, and cancellation.

Deliverables
- API design (C header and JSON wire shape) and ownership rules. ([01-interface.md](01-interface.md))
- Adapter guidelines and common patterns ([02-adapters.md](02-adapters.md)).
- Minimal starter code: `inference.h` snippet and a mock adapter example ([03-starter-code.md](03-starter-code.md)).
- Tests and benchmark plan ([04-tests-benchmarks.md](04-tests-benchmarks.md)).
- PR-sized task list with acceptance criteria ([05-pr-tasks.md](05-pr-tasks.md)).

Acceptance criteria
- Agents call a single, stable API to request completions or streams; switching backend requires changing a config/environment variable only.
- The API supports synchronous completions, streaming completions, model discovery, and cancellation.
- At least a mock adapter and an OpenAI-compatible HTTP adapter exist and pass contract tests.
- Ownership and threading rules are documented and adhered to by adapters.

Next
- See [01-interface.md](01-interface.md) for the proposed C API and JSON wire-format examples.
