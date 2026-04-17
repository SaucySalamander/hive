# Skills architecture

Components
----------
- Skill Registry: indexed catalog of available skills, metadata, health, versions.
- Skill Provider: the implementation of a skill (binary, service, plugin, or prompt template).
- Skill Runtime / Broker: mediates invocations, enforces policies (timeouts, quotas), routes requests to providers and collects telemetry.
- Agent Skill API / SDK: the client-facing API agents use to discover and invoke skills.
- Transport adapters: IPC (unix socket), HTTP/gRPC, in-process (dlopen), or process exec.

High-level invocation flow
-------------------------
1. Provider registers its manifest with the Registry (or Registry discovers provider).
2. Agent queries Registry for a skill by name, capability, or selector.
3. Agent sends an InvocationRequest to the Runtime/Broker (transport chosen by deployment).
4. Broker routes the request to the selected Provider, enforces limits, awaits result.
5. Provider executes and returns InvocationResponse; Broker records telemetry and returns to agent.

Modes
-----
- Synchronous blocking (short tasks)
- Asynchronous / fire-and-forget (background work)
- Streaming (long-running tasks or progressive results)

Design decisions
----------------
- Keep the agent API minimal: discover + invoke + subscribe (for streaming).
- Make the manifest the single source of capability metadata.
- Support incremental adapters — start with in-process and HTTP adapters.
- Enforce capability-based permissions: skills declare required capabilities and requested data.

Non-functional requirements
---------------------------
- Low latency for local skills (<200ms goal)
- Auditability and telemetry for each invocation
- Minimal attack surface: sandboxing and capability checks
- Versioning and graceful fallback to older skill versions

Error handling patterns
-----------------------
- Timeouts returned as structured errors
- Retries recommended for idempotent skills only
- Circuit-breaker on repeated failures per-provider
