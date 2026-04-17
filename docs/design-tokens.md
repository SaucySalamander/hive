# Hive Design Tokens

Canonical reference for the Hive GTK4 design system.
The authoritative source for actual values is `src/gtk/hive-theme.css`.

## Colour palette

| Token                  | Light value | Dark value  | Use |
|------------------------|-------------|-------------|-----|
| `hive_primary`         | `#F5A623`   | `#F5A623`   | Accent, CTA buttons, selection |
| `hive_primary_hover`   | `#E09510`   | `#E09510`   | Button hover |
| `hive_primary_alpha`   | 15 % amber  | 15 % amber  | Row selection background |
| `hive_surface0`        | `#FFFFFF`   | `#1C1C1E`   | Window background |
| `hive_surface1`        | `#F2F2F7`   | `#2C2C2E`   | Headerbar, sidebar, statusbar |
| `hive_surface2`        | `#E5E5EA`   | `#3A3A3C`   | Hover state, dividers |
| `hive_border`          | `#C7C7CC`   | `#48484A`   | All 1 px borders |
| `hive_text`            | `#1C1C1E`   | `#F2F2F7`   | Primary text |
| `hive_text_secondary`  | `#6C6C70`   | `#AEAEB2`   | Labels, metadata |
| `hive_success`         | `#34C759`   | `#34C759`   | OK / success indicators |
| `hive_error`           | `#FF3B30`   | `#FF3B30`   | Error / destructive actions |
| `hive_warning`         | `#FF9500`   | `#FF9500`   | Warnings |

## Spacing scale

| Step | Value | Usage |
|------|-------|-------|
| 1    | 4 px  | Icon gap, tight inline padding |
| 2    | 8 px  | Row padding, button horizontal padding |
| 3    | 12 px | Sidebar/statusbar padding |
| 4    | 16 px | Section margin |
| 5    | 24 px | Larger vertical gaps |
| 6    | 32 px | Page-level padding |

## Typography

| Role        | Family                            | Size  | Weight |
|-------------|-----------------------------------|-------|--------|
| UI default  | CothamSans → system-ui → sans-serif | 14 px | 400 |
| Window title | CothamSans                       | 15 px | 600 |
| Section header | CothamSans (caps)              | 11 px | 700 |
| Monospace (editor) | monospace                 | 13 px | 400 |
| Badge       | CothamSans                        | 11 px | 700 |

Bundled font: `assets/fonts/CothamSans.otf` — embedded via GResource at
`/org/hive/hive/fonts/CothamSans.otf`.

## Elevation / shadow

GTK4 uses platform shadows. No custom box-shadow tokens needed.
Use surface layer stacking (`surface0 → surface1 → surface2`) for depth.

## Icon strategy

- Use **symbolic icons** (`*-symbolic`) from the system icon theme wherever
  possible (Adwaita or Breeze on most distros).
- Custom icons (if needed) go in `assets/icons/` as 16 × 16 and 32 × 32 SVG
  symbolic files following the
  [freedesktop icon naming spec](https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html).
- Register via GResource under `/org/hive/hive/icons/` and load with
  `gtk_icon_theme_add_resource_path()`.

## Dark mode

Dark mode is activated by adding the CSS class `hive-dark` to the root
`GtkApplicationWindow`.  Toggle from C:

```c
if (gtk_widget_has_css_class(window, "hive-dark"))
    gtk_widget_remove_css_class(window, "hive-dark");
else
    gtk_widget_add_css_class(window, "hive-dark");
```
