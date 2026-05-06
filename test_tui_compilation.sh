#!/bin/bash
set -e

echo "Testing TUI components and pages compilation..."
CFLAGS="-D_POSIX_C_SOURCE=200809L -Isrc -DHIVE_HAVE_NCURSES=1 -std=c23 -Wall -Wextra -Werror -pedantic -pthread"

cd /home/sevenofnine/Git/hive.worktrees/feature-backend-tui

# Test individual components
echo "✓ Compiling menu component..."
cc $CFLAGS -c src/tui/components/menu.c -o /tmp/menu.o

echo "✓ Compiling input component..."
cc $CFLAGS -c src/tui/components/input.c -o /tmp/input.o

echo "✓ Compiling list component..."
cc $CFLAGS -c src/tui/components/list.c -o /tmp/list.o

echo "✓ Compiling status_bar component..."
cc $CFLAGS -c src/tui/components/status_bar.c -o /tmp/status_bar.o

# Test pages
echo "✓ Compiling dashboard page..."
cc $CFLAGS -c src/tui/pages/dashboard.c -o /tmp/dashboard.o

echo "✓ Compiling backlog page..."
cc $CFLAGS -c src/tui/pages/backlog.c -o /tmp/backlog.o

echo "✓ Compiling status page..."
cc $CFLAGS -c src/tui/pages/status.c -o /tmp/status_page.o

echo "✓ Compiling queen page..."
cc $CFLAGS -c src/tui/pages/queen.c -o /tmp/queen.o

echo "✓ Compiling tui.c..."
cc $CFLAGS -c src/tui/tui.c -o /tmp/tui.o

echo ""
echo "All TUI components and pages compiled successfully!"
echo ""
echo "Summary:"
echo "- Shared components library: 4 components (menu, input, list, status_bar)"
echo "- TUI pages: 4 pages (queen, dashboard, backlog, status)"
echo "- Total TUI additions: ~1500 LOC"
echo ""
