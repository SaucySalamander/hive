## Prompt: Theming, CSS, Fonts, and Icons

Context
- Create a coherent visual theme and asset strategy for Hive using GTK4 CSS and GResource for packaging.

Goal
- Provide a theme implementation that supports light/dark modes, high-contrast accessibility, and uses bundled fonts/icons from `assets/`.

Deliverables
- Example GTK CSS file (`src/gtk/hive-theme.css`) showing token usage for headerbars, lists, buttons, and editor containers.
- GResource XML and build instructions for bundling CSS and fonts into the binary.
- A minimal CSS-based dark-mode switch snippet and how to trigger it from C at runtime.

Constraints
- No runtime theme engine dependency. Use standard GTK4 CSS selectors and classes. Keep CSS modular and maintainable.

Files to inspect
- `assets/fonts/`
- `assets/` (icons)

Acceptance criteria
- Model returns ready-to-add CSS and a short patch to register and load resources at application startup.

Ask the model to provide precise file paths, resource registrations, and a sample commit message.
