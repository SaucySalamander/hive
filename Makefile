BUILD_DIR ?= build
GENERATOR ?= Ninja
CMAKE ?= cmake

.PHONY: all configure build clean run rebuild

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -G $(GENERATOR)

build: configure
	$(CMAKE) --build $(BUILD_DIR)

run: build
	./$(BUILD_DIR)/hive

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean >/dev/null 2>&1 || true
	rm -rf $(BUILD_DIR)

rebuild: clean build
