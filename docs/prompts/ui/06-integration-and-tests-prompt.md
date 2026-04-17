## Prompt: Integrate UI with Core Code and Add Tests

Context
- After components and theme are defined, the UI must be integrated with Hive's core systems (sessions, runtime, editor). Provide testable integration steps and small tests where appropriate.

Goal
- Produce a prioritized list of integration tasks and sample unit/integration tests or manual QA scripts to verify UI <-> core interactions.

Deliverables
- Mapping of UI actions to core APIs (e.g., open session -> `session_open()`), with proposed small glue functions and file locations.
- Example integration tests: smoke test that starts the app, opens a session, switches views, and closes cleanly (instructions for manual or automated test harness).
- Recommendations for keeping UI logic thin and delegating state to existing core modules.

Files to inspect
- `src/core/` modules (session, runtime, editor)
- `src/gtk/` bootstrap files

Acceptance criteria
- The model outputs concrete patch steps for UI-to-core wiring and at least one runnable test (or a manual QA checklist with exact steps and expected results).

Ask the model to provide example CI steps for running UI smoke tests (optional) and to keep changes minimal and review-friendly.
