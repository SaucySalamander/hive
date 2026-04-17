# Tests and benchmarks

Unit tests
----------
- Manifest parse & validation
- Registry basic operations (register, find, get)
- Invocation envelope serialization/deserialization

Integration tests
-----------------
- Spawn a local provider (process adapter) and invoke a simple skill (calculator/echo).
- Validate timeout and error semantics.
- Validate streaming invocation flows.

Performance benchmarks
----------------------
- Measure cold-start (process adapter start time).
- Measure median and p95 invocation latency for local in-process adapter.
- Measure throughput (invocations/s) under concurrency.

Suggested harness
-----------------
- Add `tests/skills/` with small C harnesses or Python scripts to start registry + provider + agent, then run scenarios.
- Add `Makefile` targets: `make test-skills`, `make bench-skills`.

CI expectations
---------------
- Unit tests must run on each PR.
- Integration tests are recommended for the main branch or nightly runs.

Metrics to capture
------------------
- Invocation latency (p50/p95/p99)
- Invocation success rate
- Provider startup time
- Resource usage per provider (memory/cpu)
