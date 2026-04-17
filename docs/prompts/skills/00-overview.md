# Skills abstraction — Overview

Purpose
-------
Provide a practical, implementable plan and reference documentation for a lightweight "skills" abstraction that agents in this codebase can use to discover, invoke, and compose reusable capability providers (skills).

Goals
-----
- Define a minimal runtime API agents call to discover and invoke skills.
- Define a portable skill manifest schema for registration and discovery.
- Specify secure invocation semantics and permission model.
- Provide examples, integration guidance, tests, and a rollout plan.

Deliverables (this folder)
--------------------------
- `01-architecture.md` — architecture and invocation flows
- `02-spec.md` — runtime API and message shapes
- `03-manifest.md` — skill manifest schema and examples
- `04-examples.md` — concrete skill examples and agent snippets
- `05-integration.md` — integration guidance for agents and adapters
- `06-tests-benchmarks.md` — tests and benchmark plans
- `07-rollout.md` — rollout and migration plan

Scope
-----
- Covers discovery, registration, invocation, error semantics, and observability.
- Targets language-agnostic skills (local process, in-process plugin, remote service, LLM-prompt skills).

Non-goals
---------
- Full implementation of runtimes or adapters (this is a design + plan docset).

Acceptance criteria
-------------------
- A documented manifest schema and validation rules.
- A clear API spec agents can implement (HTTP/gRPC/IPC examples).
- At least two example skills and agent integration snippets.
- Tests and an integration harness described for CI.

PR-sized tasks (mapped to TODO)
-----------------------------
1. Draft architecture overview
2. Specify runtime API
3. Define skill manifest schema
4. Create example skills
5. Document agent integration
6. Add tests and benchmarks
7. Plan rollout and migration
8. Create README index

Next steps
----------
Start a small prototype: implement a minimal skill registry and a single process-local "echo" or "calculator" skill to validate the invocation patterns.
