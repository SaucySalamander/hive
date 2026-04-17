# Sandbox Prompts (docs/prompts/sandbox)

This folder contains LLM/developer-friendly prompts to plan and implement sandboxing for hive agents. Use these prompts to generate designs, starter code, tests, and PR plans.

Suggested workflow

- Run `00-overview.md` then `01-goals-constraints.md` to reach consensus about scope.
- Run `02-threat-model.md` and `03-architecture.md` to choose an approach.
- Feed `06-starter-code.md` to generate an initial patch and iterate.
- Use `07-tests-benchmarks.md` and `08-pr-tasks.md` to produce CI and PR plans.

How to use

- Provide the LLM with repository context (paths listed in each prompt) and attach nearby source files.
- Ask for apply_patch-style diffs when requesting code changes.
- Keep prompts iterative: prefer many small PRs rather than a single large change.

Next steps

- If you want, I can run the first prompt (`01-goals-constraints.md`) and generate the initial goals doc now. Reply `go` to proceed.