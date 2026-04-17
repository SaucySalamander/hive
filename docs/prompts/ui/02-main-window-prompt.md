## Prompt: Implement the Main Window and Primary Layout

Role
- You are an experienced GTK4 C developer and UX engineer.

Context
- Hive is a desktop application with core logic in `src/`. Implement a modern main window that hosts the primary workflows: session list, editor, inspector, and status area.

Goal
- Provide a PR-sized implementation plan and starter patch that creates a MainWindow with a headerbar, left navigation (collapsible), central content area for editors, and a right inspector panel.

Constraints
- Use GTK4 widgets (GtkHeaderBar, GtkPaned, GtkListBox/GtkScrolledWindow, GtkGrid, GtkStack). Minimize intrusive architectural changes.

Deliverables
- A suggested file layout and new/edited files (e.g., `src/gtk/main_window.c`, header in `src/gtk/`).
- Example code snippets that initialize the main window, load theme CSS, and wire basic signals for navigation.
- A mockup (ASCII or simple layout description) and interaction flow for common tasks (open session, switch editor, focus search).

Files to inspect
- `src/gtk/gtk.c`
- `src/main.c`
- `src/core/session.c` (or equivalent) — tie UI wiring to session management.

Acceptance criteria
- A runnable patch that builds the binary and opens an empty main window with the layout described, loads the theme, and demonstrates navigation toggles.

Request the model to return a step-by-step patch plan, including exact file paths and small code edits, and to keep changes modular and testable.
