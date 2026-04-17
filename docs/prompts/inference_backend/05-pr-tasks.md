# Inference backend abstraction — PR-sized tasks

1) Draft design doc and API spec (small)
- Deliverable: The docs in this folder. Acceptance: README + interface example present. (Estimated: 1d)

2) Define public C header & ownership rules
- Deliverable: `src/core/inference.h` (finalized), `docs` updated with memory rules.
- Acceptance: header compiles in-tree; unit tests verify free/ownership behavior. (Estimated: 1-2d)

3) Implement core inference interface and adapter registration
- Deliverable: `src/core/inference.c` implementing `inf_create`/`inf_destroy`, adapter registry, JSON request marshalling.
- Acceptance: can load a registered mock adapter and route calls. (Estimated: 2-4d)

4) Add mock adapter + contract tests
- Deliverable: `tests/inference/mock_adapter.c` and contract tests (sync, stream, cancel).
- Acceptance: tests run under CI with ASAN. (Estimated: 1-2d)

5) Add OpenAI HTTP adapter + runtime config
- Deliverable: Adapter using libcurl; config parsing (env or file) and credential management.
- Acceptance: run integration test against recorded fixtures; can map logical models. (Estimated: 3-5d)

6) Add local LLM adapter (process or FFI) + benchmarks
- Deliverable: Adapter that talks to `llama.cpp`/gpt4all or loads a shared library; benchmark harness.
- Acceptance: benchmarks compare latency vs remote, pass concurrency tests. (Estimated: 3-6d)

7) Documentation, examples, and CI integration
- Deliverable: Example programs, README, and CI job for contract tests using mock adapter.
- Acceptance: CI runs tests and reports metrics. (Estimated: 1-2d)
