## Prompt: Create a Small GTK4 Component Library for Hive

Context
- Build a concise set of reusable GTK4 components to accelerate UI development and keep consistency.

Goal
- Produce a list of 6–10 high-priority components, and for each provide a small implementation plan and example C snippet.

Suggested components
- `HeaderBar` with title, search, and primary actions.
- `NavList` (sidebar) with sections and badges.
- `SessionRow` (list item) with hover and selection states.
- `EditorView` placeholder that can host text/code widgets (integration points).
- `InspectorPane` for properties with a collapsible section.
- `Dialog` helper for confirm/input modals.

Deliverables
- For each component: API surface (function names and header), GTK widget composition, CSS class names, and a tiny example file (`src/gtk/components/*.c`).
- Unit/manual test plan describing how to exercise each component (visual smoke tests).

Constraints
- Use plain C, idiomatic GTK4 patterns, avoid custom object systems beyond GObject if not necessary.

Acceptance criteria
- The model outputs clear file paths, code snippets that compile in isolation, and guidance to wire components into `main_window.c`.

Ask the model to provide minimal, copy-pasteable patches and an example commit message for the component library.
