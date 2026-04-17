# Skill manifest schema

Purpose
-------
Define a portable manifest format (YAML/JSON) that describes a skill's identity, capabilities, invocation contract, and operational constraints.

Top-level fields (recommended)
-----------------------------
- `id` (string) — stable identifier, e.g. `hive.summarizer@v1`
- `name` (string) — human-friendly name
- `version` (string) — semantic version
- `description` (string)
- `author` (object) — name, contact
- `entrypoint` (object) — how the skill is invoked (type: http|process|inproc|prompt, endpoint/command)
- `interfaces` (array) — examples of inputs/outputs, mime types
- `capabilities` (array) — tags or structured capabilities
- `permissions` (object) — data or system capabilities required
- `constraints` (object) — cpu, memory, concurrency limits
- `health` (object) — optional healthcheck config

Example (YAML)
---------------
```yaml
id: hive.calculator@v1
name: Calculator
version: 1.0.0
description: Simple arithmetic evaluator
author:
  name: infra-team
entrypoint:
  type: process
  command: ["/usr/local/bin/hive-calc"]
interfaces:
  - input: application/json
    output: application/json
capabilities:
  - arithmetic
permissions:
  allow_network: false
constraints:
  max_concurrency: 4
  timeout_ms: 2000
health:
  path: /health
  interval_ms: 10000
```

Validation rules
----------------
- `id` must be globally unique within the Registry.
- `entrypoint` must include required fields for its `type`.
- `permissions` must be explicit and deny-by-default if unspecified.

Manifest signing and provenance
------------------------------
- Optionally sign manifests to verify publisher identity.
