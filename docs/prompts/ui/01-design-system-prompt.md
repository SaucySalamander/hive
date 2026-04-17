## Prompt: Design a GTK4 Design System for Hive

Context
- Repository is a C-based application with a GTK integration layer (see `src/gtk` and `src/`). The UI will be native GTK4 using CSS for styling and GResource for bundling assets.

Goal
- Produce a compact, practical design system: color palette, spacing scale, typography choices, icon strategy, component variants, and a GTK CSS pattern that can be applied across Hive.

Constraints
- Keep changes minimal and C/GTK4-native. No new heavy dependencies. CSS-based theming only (GTK4). Support both light and dark palettes.

Deliverables (ask the model to produce)
- A small design-tokens file (JSON or Markdown) with palette, spacing, font sizes, and elevation levels.
- A GTK CSS sample (`src/gtk/hive-theme.css`) demonstrating tokens applied to headerbar, sidebar, list rows, and buttons.
- GResource / resource bundling instructions and a code snippet that registers the CSS at startup.
- A short list of recommended icon sources (SVG sprites or symbolic icons) and where to put them in `assets/`.

Files to inspect
- `src/main.c`
- `src/gtk/gtk.c`
- `src/gtk/hive_gtk.h`
- `assets/fonts/` (if present)

Acceptance criteria
- The model returns a patchable plan: files to add, code snippets to register CSS, and exact paths.
- The CSS compiles (no syntax errors) when added to the project and can be loaded via standard GTK4 resource loading.

Ask the model to output the changes as a step-by-step patch (file additions and small code edits) and an example commit message.
