# UI Implementation Prompts — Hive (GTK4)

This folder contains LLM-ready implementation prompts to design and implement a modern, useful GTK4 UI for Hive.

How to use
- For each prompt file, provide the model with the repository context (at minimum the files listed in the prompt) and ask for a PR-sized implementation.
- Prefer the "Expanded template" when requesting starter code, small patches, tests, and integration steps.

Prompt files
- 01-design-system-prompt.md — Create a GTK4 design system, tokens, and CSS strategy.
- 02-main-window-prompt.md — Implement the main window layout and primary interactions.
- 03-component-library-prompt.md — Build a small set of reusable GTK4 components.
- 04-theme-and-styling-prompt.md — Theming, fonts, icons, and resource bundling (gresources/CSS).
- 05-accessibility-performance-prompt.md — Accessibility, keyboard navigation, and performance guidance.
- 06-integration-and-tests-prompt.md — Integration with core code, tests and validation steps.

Goal
Produce reproducible LLM prompts that yield actionable, testable PRs or code patches implementing a modern GTK4 UI for Hive.
