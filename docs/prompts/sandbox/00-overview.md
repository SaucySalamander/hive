# Sandboxing for Hive agents — Implementation prompts

Purpose: a compact, LLM-friendly collection of implementation prompts to design, implement, and validate a sandboxing facility for hive agents.

Use: feed each prompt below to an LLM or a developer to get PR-sized designs, patches, tests, and checklists.

Short template (one-shot)

- Context: Hive is a C-based agent runtime. Inspect `src/core`, `src/api`, `src/inference`, `Makefile`, and `build/`.
- Goal: Design and implement a sandbox that isolates untrusted agent code, enforces capability-based policies, and preserves runtime performance.
- Constraints: Linux-first (seccomp/namespaces), minimal dependencies, memory-safety, small runtime overhead, incremental deployable changes.
- Deliverables: architecture doc, API spec, starter code patch, tests & benchmarks, PR task list, docs.

Expanded template (use for major tasks)

1) Read the code in: `src/core/*`, `src/inference/*`, `src/api/*`, `Makefile`.
2) Summarize the goal in 1 sentence.
3) Propose 3 candidate architectures with pros/cons and estimated implementation effort.
4) Pick a recommended approach and produce an actionable PR-sized plan with acceptance criteria.
5) Provide minimal starter code or an apply_patch-style diff for the first PR.
6) List tests, fuzz targets, benchmarks and CI changes required.

This folder (index)

- 01-goals-constraints.md — prompts to elicit goals, constraints and acceptance criteria.
- 02-threat-model.md — prompts for exhaustive threat modeling and mitigations.
- 03-architecture.md — prompts to generate architectures and trade-offs.
- 04-interfaces.md — prompts to define APIs, message schemas, and policy formats.
- 05-adapters.md — prompts to design platform adapters (seccomp, WASM, container, Windows alternatives).
- 06-starter-code.md — prompts to request minimal starter code and an apply_patch-style diff.
- 07-tests-benchmarks.md — prompts for tests, fuzzing, and benchmarks.
- 08-pr-tasks.md — prompts to break work into PR-sized tasks with acceptance criteria.
- README.md — usage and how to run these prompts.

How to use these prompts

- Start with `00-overview.md` and `01-goals-constraints.md` to scope the work.
- Run the `03-architecture.md` prompt to evaluate options.
- Use `06-starter-code.md` to generate a minimal patch you can iterate on.

Notes

- Keep the first implementation minimal and Linux-first so it can be integrated incrementally.
- Prefer designs that are testable and enable progressive hardening (feature flags, runtime policy).