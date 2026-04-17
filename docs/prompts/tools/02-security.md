# Security & Permissions

This document describes the security model and review process required before a tool is enabled in production.

Principles
- Principle of least privilege: tools declare required permissions and are denied by default.
- Fail-safe: on validation or policy failure, reject calls rather than execute.
- Auditability: every call must be logged with `call_id`, `agent_id`, `args` (redacted if necessary), result and outcome.

Permissions model
- Tools declare permission scope(s) in `ToolManifest.permissions`.
- Agents inherit capabilities based on runtime-assigned roles; the runtime enforces that `agent_id` has the required permission(s) to call the tool.

Sandboxing and isolation
- Tools that perform privileged actions must run isolated from core runtime (separate process, container, or restricted syscall namespace).
- Enforce resource limits (CPU, memory, ephemeral disk) and execution TTLs.

Secrets and credential handling
- Tools that need credentials must fetch them from a secure secret manager under runtime control; do not allow agents to supply raw secrets in `args`.

Input/output sanitization
- Validate `args` against the declared `inputSchema`.
- Sanitize tool outputs before presenting them to agents (mask PII if policy requires).

Review & approval process for adding tools
- Every new `ToolManifest` must be reviewed and approved by a security reviewer.
- Add a checklist: threat model, required permissions, sandboxing plan, audit retention policy, rate limits.

Logging and retention
- Log at least: `call_id`, `timestamp`, `agent_id`, `tool`, `version`, `args_redacted`, `result_status`, `duration_ms`.
- Retention and access control depend on policy; restrict logs containing sensitive data.

Rate limiting & quotas
- Per-agent and per-tool quotas should be configurable at runtime.

Incident response
- Provide a mechanism to revoke a tool, invalidate outstanding tokens, and redact logs when needed.
