## Prompt: Accessibility, Keyboard Navigation, and Performance

Context
- Ensure Hive's UI is accessible, keyboard-first, and performs well on modest hardware.

Goal
- Produce a checklist and concrete patches that improve keyboard navigation, screen-reader friendliness, and common performance hotspots for GTK4 UIs.

Deliverables
- Accessibility checklist (labels, roles, focus order, ARIA-style notes where needed).
- Example code snippets to set accessible names/roles and keyboard accelerators for primary actions.
- Guidance for profiling and a short list of likely perf issues to watch for (large lists, image/ icon handling, CSS complexity) and mitigations.

Acceptance criteria
- The model returns actionable changes (code + QA steps) that a reviewer can apply and verify using `g_accessibility` tools and manual keyboard tests.

Ask the model to provide test steps and suggested automated checks (where feasible), plus a commit message and files to add.
