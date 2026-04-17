# Overview: Tool Abstraction for Agents

Purpose
- Define a clear, simple abstraction that allows autonomous agents in this codebase to invoke external or local tools safely, consistently, and audibly.

Goals
- Standardized call envelope and response model agents use to request work.
- Strong security and permission model (least privilege by default).
- Clear testing and observable audit logs for calls.
- Incremental rollout path with a minimal reference implementation.

Non-goals
- Dictating how every tool is implemented internally (tools may use their own internal APIs).
- Replacing host-level sandboxing or OS-level security; the spec assumes those will be applied by implementers.

Audience
- LLM/agent integrators, platform engineers adding new tools, and reviewers who approve tools for production.

Constraints
- Must be language- and transport-agnostic (work with in-process plugins, HTTP/gRPC wrappers, or external services).
- Minimal runtime dependencies; prefer JSON-based envelopes for portability.

Quick next steps
- Read the API spec in `01-spec.md` and the security guidance in `02-security.md`.
- Implement a minimal in-process dispatcher and one example tool for validation.
