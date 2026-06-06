BUILD_DIR  ?= build-linux
BUILD_TYPE ?= Release
GENERATOR  ?= Ninja
JOBS       ?= $(shell nproc)

CMAKE      ?= cmake

# Bare `make` lists targets rather than building — run `make all` (or `build`)
# to actually compile.
.DEFAULT_GOAL := help

.PHONY: all configure build clean distclean rebuild test install help \
        wgpu wgpu-configure wgpu-tool wgpu-gen wgpu-tree-check \
        wgpu-test wgpu-python wgpu-show wgpu-clean

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

# ---------------------------------------------------------------------------
# WebGPU Python bindings (examples/wgpu_glue)
#
# The pipeline, stage by stage — each make target builds exactly one box:
#
#   include/webgpu.h            (the bundled Dawn C header)
#        |  libclang walk
#        v
#   wgpu_introspect             [wgpu-tool]   the parser/emitter executable
#        |  run it on the header
#        v
#   wgpu_ctypes.py  +  wgpu_tree.hpp   [wgpu-gen]   the two generated artefacts
#        |                                  \
#        |  import + size/enum checks        \  #include + static_assert
#        v                                    v
#   wgpu_ctypes_check.py                  wgpu_tree_check   [wgpu-tree-check]
#        \__________________ [wgpu-test] ___________________/
#
# These share $(BUILD_DIR) but force -DYCETL_BUILD_WGPU_GLUE=ON (the option is
# OFF by default so a normal build never needs libclang).
# ---------------------------------------------------------------------------

WGPU_DIR ?= $(BUILD_DIR)/examples/wgpu_glue
WGPU_PY  ?= $(WGPU_DIR)/wgpu_ctypes.py
WGPU_HPP ?= $(WGPU_DIR)/wgpu_tree.hpp

# Stage 0 — configure with the example turned on (cheap to re-run; idempotent).
wgpu-configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DYCETL_BUILD_WGPU_GLUE=ON

# Stage 1 — build the libclang-driven parser/emitter tool only.
wgpu-tool: wgpu-configure
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS) --target wgpu_introspect

# Stage 2 — run the tool on include/webgpu.h, emitting wgpu_ctypes.py + wgpu_tree.hpp.
wgpu-gen: wgpu-tool
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS) --target wgpu_glue_outputs

# Stage 3 — build the C++ consumer that walks the constexpr tree at compile time.
wgpu-tree-check: wgpu-gen
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS) --target wgpu_tree_check

# Whole example: tool + generated artefacts + C++ consumer.
wgpu: wgpu-tree-check

# Run both smoke tests (python ctypes import + constexpr tree static_asserts).
wgpu-test: wgpu
	ctest --test-dir $(BUILD_DIR) -R wgpu --output-on-failure

# Print where the generated Python module landed and how to import it.
wgpu-python: wgpu-gen
	@echo "Generated Python bindings: $(WGPU_PY)"
	@echo "Try it:  PYTHONPATH=$(WGPU_DIR) python3 -c 'import wgpu_ctypes as w; print(w.WGPUTextureFormat.WGPUTextureFormat_RGBA8Unorm)'"

# Peek at the head of the generated Python module.
wgpu-show: wgpu-gen
	@echo "== $(WGPU_PY) (first 40 lines) =="
	@head -n 40 $(WGPU_PY)
	@echo "   ... (full file at $(WGPU_PY))"

# Remove just the generated binding artefacts (keeps the rest of the build).
wgpu-clean:
	rm -f $(WGPU_PY) $(WGPU_HPP)

help:
	@echo "Targets:"
	@echo "  all        - configure (if needed) and build"
	@echo "  configure  - run cmake into $(BUILD_DIR)"
	@echo "  build      - build in $(BUILD_DIR)"
	@echo "  rebuild    - clean and rebuild"
	@echo "  clean      - cmake --build clean"
	@echo "  distclean  - rm -rf $(BUILD_DIR)"
	@echo "  test       - run ctest in $(BUILD_DIR)"
	@echo "  install    - cmake --install $(BUILD_DIR)"
	@echo ""
	@echo "WebGPU Python bindings (examples/wgpu_glue) — staged pipeline:"
	@echo "  wgpu-configure  - cmake with -DYCETL_BUILD_WGPU_GLUE=ON"
	@echo "  wgpu-tool       - 1. build the libclang parser (wgpu_introspect)"
	@echo "  wgpu-gen        - 2. emit wgpu_ctypes.py + wgpu_tree.hpp"
	@echo "  wgpu-tree-check - 3. build the C++ constexpr-tree consumer"
	@echo "  wgpu            - build the whole example (= wgpu-tree-check)"
	@echo "  wgpu-test       - run both smoke tests (python + constexpr)"
	@echo "  wgpu-python     - print the generated module path + import hint"
	@echo "  wgpu-show       - print the head of the generated wgpu_ctypes.py"
	@echo "  wgpu-clean      - remove just the generated .py/.hpp artefacts"
	@echo ""
	@echo "Variables (override on cmd line):"
	@echo "  BUILD_DIR  = $(BUILD_DIR)"
	@echo "  BUILD_TYPE = $(BUILD_TYPE)"
	@echo "  GENERATOR  = $(GENERATOR)"
	@echo "  JOBS       = $(JOBS)"
