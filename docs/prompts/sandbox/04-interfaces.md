# Interfaces & Policy Formats — Prompt

Short prompt

"Define the runtime and administrative interfaces for the sandbox: agent launch API, policy specification format, auditing hooks, failure semantics, and a minimal CLI for operators." 

Expanded prompt

Context: The sandbox will be used by the existing hive runtime to start and manage agents. The design must specify stable, minimal APIs suitable for C code and clear error semantics.

Tasks

1. Define the C API surface for the sandbox module (init, spawn, apply_policy, inspect, kill, metrics).
2. Specify the message schema for agent <-> supervisor control (JSON, protobuf, or compact binary), including handshake, capability tokens, and allowed syscalls/resources.
3. Propose a concise policy format (YAML/JSON) that supports allowlists, denylisted syscalls, file and network ACLs, resource limits (CPU, memory, disk), and ephemeral credential policies.
4. Define logging/audit events and required telemetry (what to log, where to send, how to redact secrets).
5. Define error codes and retry semantics for both graceful degradation and hard kills.
6. Provide example payloads and a minimal CLI to apply policies and inspect running sandboxes.

Deliverables

- API header sketch (function signatures, types).
- Policy schema (JSON Schema or brief spec) and example files.
- Example control messages for spawn and terminate operations.

Files to inspect

- `src/core/*`, `src/inference/*`, `src/api/*`