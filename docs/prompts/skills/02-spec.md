# Runtime API spec (summary)

Purpose
-------
Give agents a minimal, transport-agnostic API to discover and invoke skills.

Core concepts
-------------
- SkillDescriptor: lightweight metadata returned by the Registry.
- InvocationRequest: request envelope with id, skill id, payload, metadata, timeout.
- InvocationResponse: result, status, error info, optional streaming token.

API surface (logical)
---------------------
- Discover: `List<SkillDescriptor> find_skills(query)`
- Get: `SkillDescriptor get_skill(skill_id)`
- Invoke: `InvocationResponse invoke(InvocationRequest)`
- Subscribe: `subscribe(stream_token)` (for streaming results)

Suggested HTTP endpoints (example)
----------------------------------
- POST /v1/skills/discover { "query": {"name":"summarize"} }
- GET  /v1/skills/{skill_id}
- POST /v1/skills/{skill_id}/invoke  -> JSON body = InvocationRequest

InvocationRequest (JSON example)
--------------------------------
{
  "request_id": "uuid-v4",
  "skill_id": "hive.summarizer@v1",
  "input": { "text": "..." },
  "timeout_ms": 5000,
  "metadata": { "agent_id": "editor-42", "trace": "trace-id" }
}

InvocationResponse (JSON example)
---------------------------------
{
  "request_id": "uuid-v4",
  "skill_id": "hive.summarizer@v1",
  "status": "ok", // ok | error | timeout | accepted
  "output": { "summary": "..." },
  "error": null
}

Semantics & constraints
-----------------------
- `timeout_ms` is enforced by the Broker; providers should honor it.
- Responses must be idempotent when marked idempotent in manifest.
- Streaming: use a separate `subscribe` channel and `stream_token` issued by invocation.

Versioning
----------
- Use semantic names in skill IDs: `namespace.name@vMAJOR`.
- Registry supports matching with version ranges and fallbacks.

Security
--------
- Brokers enforce capability policies declared in manifests.
- Input/output should be validated by providers and brokers.
