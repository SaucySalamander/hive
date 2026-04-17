# Examples

This page contains small, runnable examples and agent invocation snippets to make the spec concrete.

1) Calculator skill (manifest)

See `03-manifest.md` for the sample YAML. Invocation payload example (JSON):

```json
{
  "request_id":"req-1",
  "skill_id":"hive.calculator@v1",
  "input": { "expr": "(12 / 3) + 5" },
  "timeout_ms": 1000
}
```

Expected response:

```json
{
  "request_id":"req-1",
  "skill_id":"hive.calculator@v1",
  "status":"ok",
  "output": { "value": 9 }
}
```

2) LLM prompt skill (summarizer)

Manifest entrypoint type: `prompt` — the provider is an LLM-backed adapter that accepts a prompt template and input variables.

Prompt template example:

```
Summarize the following text in one sentence:
{{text}}

Keep the summary under 30 words.
```

Agent pseudocode (language-agnostic)

```
skill = registry.find("hive.calculator")
req = InvocationRequest(skill_id=skill.id, input={expr: "1+2"}, timeout_ms=500)
res = skills.invoke(req)
if res.status == "ok":
  use res.output
else:
  handle error
```

Notes
-----
- Use `metadata` to pass agent-id, request tracing, and authorization tokens.
- For streaming skills, `invoke` returns `accepted` and a `stream_token` to subscribe.
