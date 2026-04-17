# Threat Model — Prompt

Short prompt

"Generate a detailed threat model for sandboxing hive agents. Enumerate attacker goals, capabilities, assets to protect, attack vectors, mitigations, and prioritized test cases mapping to mitigations." 

Expanded prompt

Context: Agents execute code and may request I/O, net access, local services, and native libraries via the runtime. The sandbox must protect host integrity, user data, other agents, and secrets.

Tasks

1. Produce attacker profiles (script kiddie, malicious model, compromised third-party model, supply-chain attacker, privileged insider).
2. Enumerate concrete capabilities for each profile (e.g., run arbitrary native syscalls, create /tmp files, open network sockets, escalate privileges through kernel bugs).
3. List assets to protect (host filesystem, private keys, network endpoints, other agents' memory/state, CPU quotas, API tokens).
4. For each attacker capability, list feasible attack vectors and exploit primitives.
5. Produce mitigations mapped to each vector (seccomp rules, namespaces, cgroups, capability tokens, WASM sandbox, policy enforcement, IO whitelists, ephemeral credentials).
6. Provide measurable test cases for each mitigation (e.g., attempt to open /etc/shadow should be blocked; attempt to create raw sockets blocked; large-memory allocation triggers OOM policy and graceful kill).
7. Prioritize mitigations by impact vs implementation cost.

Deliverables

- A threat-model table (attackers × capabilities × mitigations).
- Mapping to tests and CI checks.
- Short recommendation (top-3 mitigations to implement first).

Files to inspect

- `src/core/*`, `src/inference/*`, `src/api/*`