# Goals and Constraints — Prompt

Short prompt

"Context: Hive is a C-based agent runtime. Produce a concise, prioritized list of functional and non-functional goals for an agent sandbox feature. Include measurable acceptance criteria and suggest a minimal first deliverable that enables safe experimentation." 

Expanded prompt

Context: The repository is a C project that runs agents (`src/core`, `src/inference`, `Makefile`). We want a sandbox that mitigates risks from untrusted agent code while keeping runtime latency and memory overhead low.

Goal: Produce a prioritized requirements document containing:
- Functional goals (what the sandbox must do).
- Non-functional goals (performance, footprint, reliability, observability).
- Constraints (platforms, dependencies, compatibility with existing agent lifecycle).
- Trade-offs (security vs performance, usability vs isolation).
- Minimal first deliverable (smallest change that provides useful isolation).

Deliverables

- A prioritized list of 6–12 requirements with short justification.
- Acceptance criteria for each requirement (testable statements).
- Recommended minimal-first-deliverable and rollback strategy.

Files to inspect

- `src/core/*`, `src/inference/*`, `Makefile`, `README.md` (repo root)