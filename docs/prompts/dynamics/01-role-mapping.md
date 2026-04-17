Title: Role Mapping — Queen / Worker / Drone (Implementation Prompt)

Context:
- The existing `agent` type is the central execution unit. We will extend it with explicit role metadata and lifecycle semantics mapped from bee biology.

Goal:
- Define and implement `agent_role_t` and lifecycle hooks so the orchestrator can assign and manage roles: `QUEEN`, `WORKER`, `DRONE`.

Constraints:
- Minimal API surface changes; keep backward compatibility where possible.
- Memory-safety: no unbounded allocations per-agent; use pooled or stack allocations for transient data.

Deliverables:
- `src/core/agent.h`: `agent_role_t` enum and API prototypes.
- `src/core/agent.c`: role init/transition functions and unit tests.
- Test demonstrating role transitions and lifecycle (nursing → building → foraging).

Acceptance Criteria:
- Build succeeds.
- Agents report their role via `agent_get_role(agent*)`.
- Role transitions can be driven by orchestrator and are recorded in logs.

Files to inspect:
- `src/core/agent.c`
- `src/core/planner.c`
- `src/core/orchestrator.c`

Implementation details (suggested):
- Add enum:

```c
typedef enum {
    AGENT_ROLE_UNKNOWN = 0,
    AGENT_ROLE_QUEEN,
    AGENT_ROLE_WORKER,
    AGENT_ROLE_DRONE,
} agent_role_t;
```

- Add role + age fields to the agent struct (e.g., `uint32_t age_days; agent_role_t role;`).
- Provide APIs:
  - `agent_role_t agent_get_role(const agent_t *a);`
  - `int agent_set_role(agent_t *a, agent_role_t r);`
  - `void agent_increment_age(agent_t *a);`

Safety and performance notes:
- Keep per-agent footprint small. If extra state is needed for only some roles, allocate lazily and free promptly.
 
Additional guidance:
- Lazy per-role state: To limit per-agent memory, allocate role-specific state lazily (on first transition into a role) using a small pool or freelist. Return role-state to the pool on transition out or after a configurable idle timeout.
- Role-transition logging: Record transitions with `agent_id`, `prev_role`, `new_role`, `reason`, and a high-resolution timestamp. Log at `INFO` for rare, significant transitions and `DEBUG` for frequent transitions; apply rate-limiting if necessary to avoid I/O storms.

Tests:
- Unit test: create an agent, set sequence of roles, verify state and that memory usage stays bounded (run under ASAN).

Role → Task mapping (practical mapping):
- `cleaners`: handle tech-debt and small bug fixes — low-risk, fast-turnaround tasks that prepare work for builders.
- `nurses`: mentor and validate `cleaners`; inspect outputs, generate training examples, and annotate results into the knowledge graph.
- `builders & grocers`: main feature and integration work; produce deliverables (PRs, builds) and integrate forager outputs.
- `guards`: testing and review — run static analysis, contract tests, runtime checks; flag rework when invariants fail.
- `foragers`: expand the hive's knowledge graph by ingesting external sources and producing embeddings; tag provenance and rate-limit external calls.
- `drones`: experimental agents that run mutated prompts/policies on a small sample and publish proposals if they outperform baselines.

Queen & drones:
- `queen`: policy steward and user-facing interface that emits recommendations and enforces budgets; it should avoid centralized unilateral decisions. The queen may initiate spawn/retire recommendations but adoption should be decentralized via signals/quorum.

Clarifying questions:
- Should role transitions be deterministic (age-based) or event-driven (signals)? Implement both hooks.
