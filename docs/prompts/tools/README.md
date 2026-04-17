# Tool Abstraction for Agents — Docs Index

This folder contains the plan, specification, security guidance, implementation notes and starter code for a tool-abstraction layer that lets agents make safe, auditable tool calls.

Files:

- [docs/prompts/tools/00-overview.md](docs/prompts/tools/00-overview.md) — Purpose, goals, constraints, and non-goals.
- [docs/prompts/tools/01-spec.md](docs/prompts/tools/01-spec.md) — API schema, call envelope, response model, streaming semantics, and example JSON.
- [docs/prompts/tools/02-security.md](docs/prompts/tools/02-security.md) — Permissions, sandboxing, audit logging, and review process for adding tools.
- [docs/prompts/tools/03-implementation.md](docs/prompts/tools/03-implementation.md) — Architecture, registry, dispatcher, and phased rollout tasks.
- [docs/prompts/tools/04-tests.md](docs/prompts/tools/04-tests.md) — Testing, fuzzing, and benchmark plan.
- [docs/prompts/tools/05-starter-code.md](docs/prompts/tools/05-starter-code.md) — Minimal starter code snippets and example prompts.

How to use:

1. Read `00-overview.md` to align on goals.
2. Use `01-spec.md` as the canonical API to implement the dispatcher and tools.
3. Follow `02-security.md` before enabling tools in production.

If you want, I can: implement a reference dispatcher, add a C-based starter harness in `src/`, or open a draft PR with these docs. Which next step do you prefer?
