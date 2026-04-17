# Integration guidance for agents

Where to place code
-------------------
- Add a small `skills` module under `src/core/skills.*` (or `src/skills/*`) that implements the Agent SDK surface and adapter registration.
- Create provider adapters in `src/skills/adapters/` (http_adapter.c, process_adapter.c, inproc_adapter.c).

Agent-side SDK (minimal)
------------------------
- `skills_discover(query)` -> list of SkillDescriptor
- `skills_get(skill_id)` -> SkillDescriptor
- `skills_invoke(InvocationRequest)` -> InvocationResponse
- `skills_subscribe(stream_token, handler)` -> streaming callbacks

Adapter responsibilities
------------------------
- Translate transport-specific requests and responses to InvocationRequest/InvocationResponse.
- Enforce timeouts and per-invocation constraints.
- Surface provider health and metrics to Registry.

Deployment patterns
-------------------
- Single-binary agents: use in-process adapters for lowest latency.
- Multi-process: use unix-socket or HTTP adapters for stronger isolation.
- Remote providers: use REST/gRPC adapters with TLS and auth.

Security and permissions
------------------------
- Agents must authenticate to the Broker/Registry when required.
- Broker should validate that an agent has permission to call requested skill capabilities.

Minimal integration checklist
---------------------------
1. Add SDK skeleton to `src/core/skills.h/c`.
2. Add simple process adapter that execs a binary and exchanges JSON over stdin/stdout.
3. Add a Registry mock for unit tests.
4. Add a small example agent call (see `04-examples.md`).
