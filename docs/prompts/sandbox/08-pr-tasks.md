# PR-sized Tasks — Prompt

Short prompt

"Break the sandbox project into 6–9 PR-sized tasks. For each task produce a short description, acceptance criteria, required files changed, and a rough effort estimate (small/medium/large)." 

Expanded prompt

Context: The goal is to produce reviewable PRs that can be landed incrementally.

Example PR breakdown (the prompt should return a similar or improved list):

1. PR: `sandbox: core API and stub` (small)
   - Changes: `src/core/sandbox.h`, `src/core/sandbox.c` (stubs), `Makefile` entries, README.
   - Acceptance: builds clean and sandbox API can be invoked in tests (no real seccomp rules applied yet).

2. PR: `sandbox: linux-seccomp adapter (opt-in)` (medium)
   - Changes: implement Linux adapter, example seccomp rules, tests that block a sample syscall.
   - Acceptance: blocked syscall test fails safely; sandbox can be enabled via runtime flag.

3. PR: `sandbox: policy format and parser` (small)
   - Changes: policy JSON schema, parser, tests for malformed policies.
   - Acceptance: parser validates policies and rejects invalid input.

4. PR: `sandbox: instrumentation and audit logging` (small)
   - Changes: logging hooks, telemetry events, redact rules.
   - Acceptance: audit events emitted for spawn/kill/policy-violations.

5. PR: `sandbox: WASM adapter prototype` (large)
   - Changes: integrate Wasmtime or similar, mapping host APIs to capabilities.
   - Acceptance: simple workload runs in WASM with identical policy semantics.

6. PR: `sandbox: CI tests and benchmarks` (small/medium)
   - Changes: CI job definitions, test scripts, benchmark harness.
   - Acceptance: CI runs unit+integration and reports benchmark metrics.

Deliverables from the prompt

- A numbered list like above with acceptance criteria and files to change.
- Suggest reviewers/labels and a short release note for each PR.

Files to inspect

- `src/core/*`, `Makefile`, `docs/`