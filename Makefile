BUILD_DIR  ?= build-linux
BUILD_TYPE ?= Release
GENERATOR  ?= Ninja
JOBS       ?= $(shell nproc)

CMAKE      ?= cmake

.PHONY: all configure build clean distclean rebuild test install help

all: build

configure: $(BUILD_DIR)/CMakeCache.txt

$(BUILD_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(BUILD_DIR) -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS)

rebuild:
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS) --clean-first

clean:
	@if [ -d "$(BUILD_DIR)" ]; then $(CMAKE) --build $(BUILD_DIR) --target clean; fi

distclean:
	rm -rf $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

install: build
	$(CMAKE) --install $(BUILD_DIR)

help:
	@echo "Targets:"
	@echo "  all        - configure (if needed) and build (default)"
	@echo "  configure  - run cmake into $(BUILD_DIR)"
	@echo "  build      - build in $(BUILD_DIR)"
	@echo "  rebuild    - clean and rebuild"
	@echo "  clean      - cmake --build clean"
	@echo "  distclean  - rm -rf $(BUILD_DIR)"
	@echo "  test       - run ctest in $(BUILD_DIR)"
	@echo "  install    - cmake --install $(BUILD_DIR)"
	@echo ""
	@echo "Variables (override on cmd line):"
	@echo "  BUILD_DIR  = $(BUILD_DIR)"
	@echo "  BUILD_TYPE = $(BUILD_TYPE)"
	@echo "  GENERATOR  = $(GENERATOR)"
	@echo "  JOBS       = $(JOBS)"
