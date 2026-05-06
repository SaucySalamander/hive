CC ?= cc
BUILD_DIR ?= build
SRC_DIR := src
TARGET := $(BUILD_DIR)/hive
BUILD_SENTINEL := $(BUILD_DIR)/.dir

HIVE_ENABLE_TUI ?= auto
HIVE_ENABLE_API ?= auto
HIVE_ENABLE_SYSLOG ?= auto
HIVE_ENABLE_RAYGUI ?= 0
HIVE_ENABLE_GTK4 ?= auto

SOURCES := $(shell find $(SRC_DIR) -type f -name '*.c' | sort)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
DEPFILES := $(OBJECTS:.o=.d)

COMMON_CPPFLAGS := -D_POSIX_C_SOURCE=200809L -I$(SRC_DIR)
COMMON_CFLAGS := -std=c23 -Wall -Wextra -Werror -pedantic -pthread
COMMON_LDLIBS := -pthread -lm

ARGP_AVAILABLE := $(shell printf '%s\n' '#define _GNU_SOURCE' '#include <argp.h>' 'int main(void) { return 0; }' | $(CC) $(COMMON_CPPFLAGS) -x c - -c -o /dev/null >/dev/null 2>&1 && echo 1 || echo 0)
SYSLOG_AVAILABLE := $(shell printf '%s\n' '#include <syslog.h>' 'int main(void) { return 0; }' | $(CC) $(COMMON_CPPFLAGS) -x c - -c -o /dev/null >/dev/null 2>&1 && echo 1 || echo 0)

NCURSES_PKG := $(strip $(shell for pkg in ncursesw ncurses curses; do if pkg-config --exists $$pkg 2>/dev/null; then echo $$pkg; break; fi; done))
UV_PKG := $(strip $(shell for pkg in libuv uv; do if pkg-config --exists $$pkg 2>/dev/null; then echo $$pkg; break; fi; done))
CJSON_PKG := $(strip $(shell for pkg in cjson cJSON; do if pkg-config --exists $$pkg 2>/dev/null; then echo $$pkg; break; fi; done))

NCURSES_CFLAGS := $(if $(NCURSES_PKG),$(shell pkg-config --cflags $(NCURSES_PKG) 2>/dev/null))
NCURSES_LIBS := $(if $(NCURSES_PKG),$(shell pkg-config --libs $(NCURSES_PKG) 2>/dev/null))
UV_CFLAGS := $(if $(UV_PKG),$(shell pkg-config --cflags $(UV_PKG) 2>/dev/null))
UV_LIBS := $(if $(UV_PKG),$(shell pkg-config --libs $(UV_PKG) 2>/dev/null))
CJSON_CFLAGS := $(if $(CJSON_PKG),$(shell pkg-config --cflags $(CJSON_PKG) 2>/dev/null))
CJSON_LIBS := $(if $(CJSON_PKG),$(shell pkg-config --libs $(CJSON_PKG) 2>/dev/null))
RAYLIB_PKG := $(strip $(shell for pkg in raylib; do if pkg-config --exists $$pkg 2>/dev/null; then echo $$pkg; break; fi; done))
RAYLIB_CFLAGS := $(if $(RAYLIB_PKG),$(shell pkg-config --cflags $(RAYLIB_PKG) 2>/dev/null))
RAYLIB_LIBS := $(if $(RAYLIB_PKG),$(shell pkg-config --libs $(RAYLIB_PKG) 2>/dev/null))
GTK_PKG := $(strip $(shell for pkg in gtk4; do if pkg-config --exists $$pkg 2>/dev/null; then echo $$pkg; break; fi; done))
GTK_CFLAGS := $(if $(GTK_PKG),$(shell pkg-config --cflags $(GTK_PKG) 2>/dev/null))
GTK_LIBS := $(if $(GTK_PKG),$(shell pkg-config --libs $(GTK_PKG) 2>/dev/null))

ifeq ($(HIVE_ENABLE_TUI),auto)
HIVE_HAVE_NCURSES := $(if $(NCURSES_PKG),1,0)
else ifeq ($(HIVE_ENABLE_TUI),1)
ifeq ($(NCURSES_PKG),)
$(error HIVE_ENABLE_TUI=1 requested but no ncurses package was found)
endif
HIVE_HAVE_NCURSES := 1
else
HIVE_HAVE_NCURSES := 0
endif

# RayGUI (raylib) optional support (disabled by default)
ifeq ($(HIVE_ENABLE_RAYGUI),auto)
HIVE_HAVE_RAYGUI := $(if $(RAYLIB_PKG),1,0)
else ifeq ($(HIVE_ENABLE_RAYGUI),1)
ifeq ($(RAYLIB_PKG),)
$(error HIVE_ENABLE_RAYGUI=1 requested but no raylib package was found)
endif
HIVE_HAVE_RAYGUI := 1
else
HIVE_HAVE_RAYGUI := 0
endif

# GTK4 optional support
ifeq ($(HIVE_ENABLE_GTK4),auto)
HIVE_HAVE_GTK4 := $(if $(GTK_PKG),1,0)
else ifeq ($(HIVE_ENABLE_GTK4),1)
ifeq ($(GTK_PKG),)
$(error HIVE_ENABLE_GTK4=1 requested but no gtk4 package was found)
endif
HIVE_HAVE_GTK4 := 1
else
HIVE_HAVE_GTK4 := 0
endif

ifeq ($(HIVE_ENABLE_API),auto)
HIVE_HAVE_API := $(if $(UV_PKG),$(if $(CJSON_PKG),1,0),0)
else ifeq ($(HIVE_ENABLE_API),1)
ifeq ($(UV_PKG),)
$(error HIVE_ENABLE_API=1 requested but no libuv package was found)
endif
ifeq ($(CJSON_PKG),)
$(error HIVE_ENABLE_API=1 requested but no cJSON package was found)
endif
HIVE_HAVE_API := 1
else
HIVE_HAVE_API := 0
endif

ifeq ($(HIVE_ENABLE_SYSLOG),auto)
HIVE_HAVE_SYSLOG := $(SYSLOG_AVAILABLE)
else ifeq ($(HIVE_ENABLE_SYSLOG),1)
ifeq ($(SYSLOG_AVAILABLE),0)
$(error HIVE_ENABLE_SYSLOG=1 requested but syslog.h was not found)
endif
HIVE_HAVE_SYSLOG := 1
else
HIVE_HAVE_SYSLOG := 0
endif

CPPFLAGS += $(COMMON_CPPFLAGS)
CPPFLAGS += -DHIVE_HAVE_ARGP=$(ARGP_AVAILABLE)
CPPFLAGS += -DHIVE_HAVE_SYSLOG=$(HIVE_HAVE_SYSLOG)
CPPFLAGS += -DHIVE_HAVE_NCURSES=$(HIVE_HAVE_NCURSES)
CPPFLAGS += -DHIVE_HAVE_LIBUV=$(HIVE_HAVE_API)
CPPFLAGS += -DHIVE_HAVE_CJSON=$(HIVE_HAVE_API)
CPPFLAGS += -DHIVE_HAVE_RAYGUI=$(HIVE_HAVE_RAYGUI)
CPPFLAGS += -DHIVE_HAVE_GTK4=$(HIVE_HAVE_GTK4)

CFLAGS += $(COMMON_CFLAGS)
LDLIBS += $(COMMON_LDLIBS)

ifeq ($(HIVE_HAVE_NCURSES),1)
CPPFLAGS += $(NCURSES_CFLAGS)
LDLIBS += $(NCURSES_LIBS)
endif

ifeq ($(HIVE_HAVE_RAYGUI),1)
CPPFLAGS += $(RAYLIB_CFLAGS)
LDLIBS += $(RAYLIB_LIBS)
endif

ifeq ($(HIVE_HAVE_GTK4),1)
GRESOURCE_XML   := $(SRC_DIR)/gtk/hive.gresource.xml
GRESOURCE_C     := $(BUILD_DIR)/gtk/hive_resources.c
GRESOURCE_O     := $(BUILD_DIR)/gtk/hive_resources.o
OBJECTS         += $(GRESOURCE_O)
CPPFLAGS        += $(GTK_CFLAGS)
LDLIBS          += $(GTK_LIBS)
endif

ifeq ($(HIVE_HAVE_API),1)
CPPFLAGS += $(UV_CFLAGS) -I.
LDLIBS += $(UV_LIBS) -L. -lcjson
endif

.PHONY: all build run clean rebuild

all: build

build: $(TARGET)

$(TARGET): $(OBJECTS) | $(BUILD_SENTINEL)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_SENTINEL):
	mkdir -p $(BUILD_DIR)
	touch $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_SENTINEL)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: build
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build

# ----------------------------------------------------------------
# GResource compilation (GTK4 only)
# Embeds hive-theme.css and assets/fonts/CothamSans.otf into the binary.
# ----------------------------------------------------------------
ifeq ($(HIVE_HAVE_GTK4),1)
$(GRESOURCE_C): $(GRESOURCE_XML) \
                $(SRC_DIR)/gtk/hive-theme.css \
                assets/fonts/CothamSans.otf \
                | $(BUILD_SENTINEL)
	mkdir -p $(@D)
	glib-compile-resources \
	    --generate-source \
	    --target=$@ \
	    --sourcedir=$(SRC_DIR)/gtk \
	    --sourcedir=assets/fonts \
	    $<

# Compile the generated C without the project's strict warning flags to
# avoid -Werror failures on auto-generated code.
$(GRESOURCE_O): $(GRESOURCE_C) | $(BUILD_SENTINEL)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c $< -o $@
endif

-include $(DEPFILES)
