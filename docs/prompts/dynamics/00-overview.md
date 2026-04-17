Title: Hive Dynamics — Overview (Implementation Prompt)

Context:
- Repository: `hive` — a C-based AI agent harness. Key code lives under `src/`.
- Goal: Apply biological hive dynamics (queen, workers, drones, pheromones, waggle dance, quorum) to orchestrating AI agents in this harness.

Goal:
- Produce a design and a small set of PR-sized changes that implement a first-class orchestration model inspired by bee-hive dynamics. Deliver runnable starter code and tests.

Constraints:
- Language: C (primary). Prefer minimal dependencies and no runtime GC.
- Safety: Memory-safe idioms (bounded buffers, explicit ownership, sanitizer-friendly code).
- Performance: Low-latency message passing and low contention; provide microbenchmarks.

Deliverables:
- Design doc and API sketches.
- 3–6 PR-sized implementation patches (small, reviewable).
- Minimal starter code (message/signal API, role metadata in agents).
- Unit tests + a microbenchmark harness and sanitizer guidance.

Acceptance Criteria:
- All builds succeed (`make` on repo root).
- New unit tests pass.
- Code passes ASAN/UBSAN (no leaks / undefined behavior).
- Microbenchmark demonstrates target throughput (submit measurements in PR).

Files to inspect (start here):
- `src/core/orchestrator.c`
- `src/core/agent.c`
- `src/core/planner.c`
- `src/core/session.c`
- `src/common/strings.c`

Implementation Steps (high level):
1) Read the listed files and propose design (1–2 pages). Commit design as `docs/architecture/hive-dynamics.md`.
2) Add a `role` field and lifecycle hooks to the agent struct (queen/worker/drone). Small patch.
3) Implement a lightweight signal bus for pheromones and waggle-dance adverts (publish/subscribe API).
4) Integrate the signal bus with the orchestrator: interpret adverts, maintain a short-term pheromone map, and trigger agent behavior.
5) Add quorum decision logic for multi-agent choices (swarm move / resource allocation).
6) Add tests and a microbenchmark; run with ASAN/UBSAN and report results.

Starter notes:
- Prefer a mutex+condition-variable bounded queue for first pass (simple and safe). Optimize with atomics/lock-free structures only after benchmarks.
- Use compile-time flags to enable sanitizers and benchmarking modes.

Risks & required clarifications:
- Per-agent memory costs: Adding role fields to every agent increases footprint at scale. Recommend lazy per-role allocations or a pooled per-role state to keep per-agent memory bounded.
- Effective queue capacity: The `bqueue` sketch uses a ring-buffer idiom that leaves one slot empty (effective capacity = requested cap - 1). Either allocate an internal buffer of `capacity + 1` or document the semantics clearly so callers get the capacity they expect.
- Ownership semantics: Clarify who owns memory for queue items and signal payloads. Recommended policy: `signal_publish()` should copy payload into a pooled signal buffer; subscribers receive a const pointer and must not free it. For `bqueue`, `bqueue_push()` should transfer ownership to the queue; the consumer is responsible for freeing the popped item unless a copy API is provided.
- Timing/expiry defaults: Pheromone decay and quorum time windows need explicit numeric defaults and tuning guidance. Suggested, tunable defaults (examples): `PHEROMONE_TTL_MS=5000`, `WAGGLE_TTL_MS=15000`, `QUORUM_WINDOW_MS=200` — validate and tune these with benchmarks.
- Concurrency model: The prompts leave event-loop vs multi-thread trade-offs open. For the first pass, prefer a single-threaded event-loop or a small-thread-pool model to limit complexity; justify the choice in the design doc.

These clarifications are reflected across the role-mapping, architecture, starter-code, tasks, and tests prompts below.

Clarifying questions to ask before implementation:
- Target throughput and latency goals? (e.g., messages/sec, acceptable latency)
- Preferred concurrency model (single-threaded event loop vs. multi-threaded agents)?
- Are persistent storage or networked agents required in first pass?
