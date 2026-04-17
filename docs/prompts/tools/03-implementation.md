# Implementation Plan & Architecture

High level components
- Tool Registry: stores `ToolManifest`s and health/metadata.
- Dispatcher / Router: receives `ToolCall`s, validates them, authorizes the agent, and routes to registered tool implementations.
- Tool Adapter: a small wrapper each tool implements to adapt the tool to the runtime (in-process function, HTTP/gRPC wrapper, or plugin shim).
- Audit sink: append-only log storage for call events.

Phased rollout

1. Reference in-process dispatcher (MVP)
   - In-memory registry, single-threaded dispatcher.
   - One example tool (read-only) implemented as a library call.
   - Basic validation and logging to local file.

2. Extend to external adapters
   - Add HTTP/gRPC adapter support.
   - Implement healthchecks and per-tool timeouts.

3. Harden security and sandboxing
   - Move privileged tools into separate processes/containers.
   - Integrate with secret manager and RBAC provider.

4. Production rollout
   - Add quotas, metrics, monitoring, and structured audit store.
   - Training and docs for reviewers.

Developer API (conceptual)

```c
// Register a tool manifest
int register_tool(const char* manifest_json);

// Dispatcher entry point (blocking for sync calls)
int dispatch_tool_call(const char* toolcall_json, char** out_response_json);
```

Health & observability
- Per-tool health endpoints; metrics: calls_total, calls_failed, avg_latency_ms.

Tool lifecycle
- Register, enable, disable, retire. Registry holds `enabled` flag and rollout metadata.

Acceptance tests for implementation
- Dispatcher validates input against `inputSchema` and rejects invalid calls.
- Authorized agent succeeds; unauthorized agent receives `PERMISSION_DENIED`.
- A sample streaming call emits ordered frames readable by the agent.
