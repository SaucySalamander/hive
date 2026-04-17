# API Spec: Tool Call Envelope and Models

This document is the canonical API for how agents format tool calls, how tools declare their contracts, and how the runtime responds.

Terminology
- Tool: A capability registered in the runtime (local binary, plugin, or remote service).
- ToolManifest: Metadata describing a tool (name, id, input/output schemas, permissions).
- ToolCall: The canonical request envelope an agent emits to invoke a tool.
- ToolResponse: The canonical response from the runtime/tool.

Core types (JSON-first)

- ToolManifest

```json
{
  "id": "calendar.create_event",
  "name": "Calendar: Create Event",
  "description": "Create a calendar event for a user",
  "version": "1.0",
  "inputSchema": {"$ref":"#/definitions/CreateEvent"},
  "outputSchema": {"type":"object"},
  "permissions": ["calendar:write"],
  "callTypes": ["sync","async","stream"],
  "timeoutMsDefault": 30000
}
```

- ToolCall (agent -> runtime)

```json
{
  "call_id": "uuid-v4",
  "tool": "calendar.create_event",
  "version": "1.0",
  "call_type": "sync",
  "args": {"title":"Standup","when":"2026-04-16T09:00:00Z"},
  "timeout_ms": 20000,
  "meta": {"agent_id":"planner-1","trace_id":"..."}
}
```

- ToolResponse (runtime/tool -> agent)

```json
{
  "call_id": "uuid-v4",
  "status": "ok", // ok | error | in_progress
  "result": {"event_id":"evt_123"},
  "error": null,
  "meta": {"duration_ms":123}
}
```

Streaming semantics
- If `call_type` is `stream`, the runtime emits an ordered sequence of frames. Each frame is a JSON object with `call_id` and `frame_type` (progress|partial|done|error) and a `payload` field.

Validation and schemas
- `ToolManifest` should include JSON Schema references for `inputSchema`/`outputSchema`.
- The runtime must validate `args` against `inputSchema` before dispatch, rejecting malformed input with a clear `400`-style error in `ToolResponse.error`.

Error model
- Use structured errors: `{code, message, details}`. Prefer stable machine-readable codes (e.g., `TOOL_NOT_FOUND`, `INVALID_ARGS`, `TIMEOUT`, `PERMISSION_DENIED`).

Discovery and versioning
- Agents should be able to call a local `/.well-known/tools` endpoint or query the runtime registry to discover available `ToolManifest`s.
- `ToolManifest.version` must follow semver; runtime should enforce compatibility.

Call examples and agent prompt conventions
- When prompting an LLM-based agent, instruct it to produce a single JSON `ToolCall` object (no extraneous commentary). Provide a short template in `05-starter-code.md`.

Acceptance criteria for spec
- Implementations must round-trip a `ToolCall` with the registry, produce validated `ToolResponse`s, and log call/audit records with `call_id`, `agent_id`, timestamp, and outcome.
