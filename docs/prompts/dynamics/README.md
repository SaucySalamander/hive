Overview
--------
This folder contains implementation prompts that map bee-hive dynamics to AI-agent orchestration for the `hive` harness. Use these prompts with the Implementation Prompt Designer agent (or paste into an issue/PR) to produce design docs, PR-sized tasks, and starter code.

How to use
----------
- Read `00-overview.md` to get the high-level goal and constraints.
- Pick a prompt file (role mapping, architecture, PR tasks, starter code, or tests).
- Give the prompt to the Implementation Prompt Designer agent or a developer and request a patch.

- See `06-spawn-lifecycle.md` for spawn rules, lifecycle policies, drone experiment guidance, and pseudocode to implement spawn/retire behavior.

Example agent prompt
--------------------
"Context: Local repo is a C-based agent harness (paths in repository). Goal: Implement hive-inspired orchestration (pheromones, waggle dance, quorum). Constraints: C-first, memory-safety, high-performance. Deliverables: design, PRs, starter code, tests. Files to inspect: src/core/orchestrator.c, src/core/agent.c, src/core/planner.c."

Notes
-----
- These prompts are action-oriented and include starter snippets. They are designed to produce small, reviewable patches that emphasize memory-safety and measurable performance.
