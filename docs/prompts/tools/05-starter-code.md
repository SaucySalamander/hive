# Starter Code & Agent Prompt Templates

This file contains minimal examples to get started. The spec is transport-agnostic; examples below use JSON envelopes.

Agent prompt template (for LLM-driven agents)

```
You are an agent that may call runtime tools. If you decide to call a tool, emit exactly one JSON object matching the ToolCall schema and nothing else. Example:

{"call_id":"<uuid>","tool":"calendar.create_event","version":"1.0","call_type":"sync","args":{"title":"Standup","when":"2026-04-16T09:00:00Z"}}

If the tool completes, wait for the runtime response and continue.
```

Minimal dispatcher pseudo-code (conceptual)

```c
// parse JSON -> validate against inputSchema -> authorize -> invoke tool
int dispatch_tool_call(const char* toolcall_json, char** out_response_json) {
  // 1) parse and validate
  // 2) check registry for tool and input schema
  // 3) authorize agent_id from toolcall.meta
  // 4) invoke adapter and collect response
  // 5) write audit record and return response JSON
}
```

Example ToolCall JSON (full)

```json
{
  "call_id": "1111-2222-3333",
  "tool": "echo.print",
  "version": "1.0",
  "call_type": "sync",
  "args": {"text":"hello"},
  "meta": {"agent_id":"test-agent"}
}
```

Local test harness (bash)

```sh
# send a ToolCall JSON to the dispatcher test endpoint
curl -X POST -H "Content-Type: application/json" --data @call.json http://localhost:9000/v1/toolcall
```

Next steps (starter implementation)
- Implement `dispatch_tool_call` in a small C module under `src/` using an embedded JSON lib (cJSON or similar), then wire a single example tool.
