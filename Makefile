BUILD_DIR  ?= build-linux
BUILD_TYPE ?= Release
GENERATOR  ?= Ninja
JOBS       ?= $(shell nproc)

CMAKE      ?= cmake

# Bare `make` lists targets rather than building — run `make all` (or `build`)
# to actually compile.
.DEFAULT_GOAL := help

.PHONY: all configure build clean distclean rebuild test install help \
        webgpu webgpu-configure webgpu-tool webgpu-gen webgpu-tree-check \
        webgpu-test webgpu-python webgpu-show webgpu-clean \
        webgpu-dawn webgpu-native

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
# WebGPU Python bindings (examples/webgpu)
#
# The pipeline, stage by stage — each make target builds exactly one box:
#
#   <impl>/include/webgpu.h     (the bundled C header; impl = dawn | native)
#        |  libclang walk
#        v
#   webgpu_introspect           [webgpu-tool]   the parser/emitter executable
#        |  run it on the header
#        v
#   webgpu_ctypes.py + webgpu_tree.hpp  [webgpu-gen]  the two generated artefacts
#        |                                  \
#        |  import + size/enum checks        \  #include + static_assert
#        v                                    v
#   webgpu_ctypes_check.py                webgpu_tree_check  [webgpu-tree-check]
#        \________________ [webgpu-test] ___________________/
#
# These share $(BUILD_DIR) but force -DYCETL_BUILD_WEBGPU=ON (the option is
# OFF by default so a normal build never needs libclang). Choose the header
# implementation with WEBGPU_IMPL=dawn|native (default dawn).
# ---------------------------------------------------------------------------

WEBGPU_IMPL ?= dawn
WEBGPU_DIR  ?= $(BUILD_DIR)/examples/webgpu
WEBGPU_PY   ?= $(WEBGPU_DIR)/webgpu_ctypes.py
WEBGPU_HPP  ?= $(WEBGPU_DIR)/webgpu_tree.hpp

# Stage 0 — configure with the example turned on (cheap to re-run; idempotent).
# CMAKE_EXTRA lets the per-impl wrappers below pass extra flags (e.g. trim the
# build to the example only).
webgpu-configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DYCETL_BUILD_WEBGPU=ON \
		-DWEBGPU_IMPL=$(WEBGPU_IMPL) $(CMAKE_EXTRA)

# Stage 1 — build the libclang-driven parser/emitter tool only.
webgpu-tool: webgpu-configure
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS) --target webgpu_introspect

# Stage 2 — run the tool on <impl>/include/webgpu.h, emitting the .py + .hpp.
webgpu-gen: webgpu-tool
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS) --target webgpu_outputs

# Stage 3 — build the C++ consumer that walks the constexpr tree at compile time.
webgpu-tree-check: webgpu-gen
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS) --target webgpu_tree_check

# Whole example: tool + generated artefacts + C++ consumer.
webgpu: webgpu-tree-check

# Run both smoke tests (python ctypes import + constexpr tree static_asserts).
webgpu-test: webgpu
	ctest --test-dir $(BUILD_DIR) -R webgpu --output-on-failure

# Print where the generated Python module landed and how to import it.
webgpu-python: webgpu-gen
	@echo "Generated Python bindings: $(WEBGPU_PY)"
	@echo "Try it:  PYTHONPATH=$(WEBGPU_DIR) python3 -c 'import webgpu_ctypes as w; print(w.WGPUTextureFormat.WGPUTextureFormat_RGBA8Unorm)'"

# Peek at the head of the generated Python module.
webgpu-show: webgpu-gen
	@echo "== $(WEBGPU_PY) (first 40 lines) =="
	@head -n 40 $(WEBGPU_PY)
	@echo "   ... (full file at $(WEBGPU_PY))"

# Remove just the generated binding artefacts (keeps the rest of the build).
webgpu-clean:
	rm -f $(WEBGPU_PY) $(WEBGPU_HPP)

# --- Per-implementation variants (dawn / native) ---------------------------
# Each implementation gets its OWN build tree so the two can coexist and be
# compared without clobbering each other's generated module. The trees are
# nested under $(BUILD_DIR) (already gitignored) and trimmed to the example
# only (no unit_test / yce). Every stage of the generic family above is
# available per impl, e.g.:
#
#   make webgpu-dawn-configure     make webgpu-native-configure
#   make webgpu-dawn-test          make webgpu-native-test
#   make webgpu-dawn-show          make webgpu-native-show
#   make webgpu-dawn               make webgpu-native      (build whole example)
#
# Implemented by re-invoking the generic webgpu-<stage> target with the impl
# and a dedicated build dir overridden on the command line.

WEBGPU_LEAN := -DYCETL_BUILD_TESTS=OFF -DYCETL_BUILD_EXAMPLES=OFF

webgpu-dawn-%:
	@$(MAKE) --no-print-directory webgpu-$* \
		WEBGPU_IMPL=dawn BUILD_DIR=$(BUILD_DIR)/webgpu-dawn \
		CMAKE_EXTRA='$(WEBGPU_LEAN)'

webgpu-native-%:
	@$(MAKE) --no-print-directory webgpu-$* \
		WEBGPU_IMPL=native BUILD_DIR=$(BUILD_DIR)/webgpu-native \
		CMAKE_EXTRA='$(WEBGPU_LEAN)'

# Plain `webgpu-dawn` / `webgpu-native` build the whole example for that impl.
webgpu-dawn:
	@$(MAKE) --no-print-directory webgpu \
		WEBGPU_IMPL=dawn BUILD_DIR=$(BUILD_DIR)/webgpu-dawn \
		CMAKE_EXTRA='$(WEBGPU_LEAN)'

webgpu-native:
	@$(MAKE) --no-print-directory webgpu \
		WEBGPU_IMPL=native BUILD_DIR=$(BUILD_DIR)/webgpu-native \
		CMAKE_EXTRA='$(WEBGPU_LEAN)'

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
	@echo "WebGPU Python bindings (examples/webgpu) — two independent targets,"
	@echo "each parses its own header into its own build tree:"
	@echo ""
	@echo "  DAWN   header examples/webgpu/dawn/include/webgpu.h   tree $(BUILD_DIR)/webgpu-dawn"
	@echo "    webgpu-dawn-configure   - cmake (-DYCETL_BUILD_WEBGPU=ON, WEBGPU_IMPL=dawn)"
	@echo "    webgpu-dawn-tool        - build the libclang parser (webgpu_introspect)"
	@echo "    webgpu-dawn-gen         - emit webgpu_ctypes.py + webgpu_tree.hpp"
	@echo "    webgpu-dawn-tree-check  - build the C++ constexpr-tree consumer"
	@echo "    webgpu-dawn             - build the whole example"
	@echo "    webgpu-dawn-test        - run both smoke tests (python + constexpr)"
	@echo "    webgpu-dawn-python      - print the generated module path + import hint"
	@echo "    webgpu-dawn-show        - print the head of the generated module"
	@echo "    webgpu-dawn-clean       - remove the generated .py/.hpp"
	@echo ""
	@echo "  NATIVE header examples/webgpu/native/include/webgpu.h tree $(BUILD_DIR)/webgpu-native"
	@echo "    webgpu-native-configure - cmake (-DYCETL_BUILD_WEBGPU=ON, WEBGPU_IMPL=native)"
	@echo "    webgpu-native-tool      - build the libclang parser (webgpu_introspect)"
	@echo "    webgpu-native-gen       - emit webgpu_ctypes.py + webgpu_tree.hpp"
	@echo "    webgpu-native-tree-check- build the C++ constexpr-tree consumer"
	@echo "    webgpu-native           - build the whole example"
	@echo "    webgpu-native-test      - run both smoke tests (python + constexpr)"
	@echo "    webgpu-native-python    - print the generated module path + import hint"
	@echo "    webgpu-native-show      - print the head of the generated module"
	@echo "    webgpu-native-clean     - remove the generated .py/.hpp"
	@echo ""
	@echo "Variables (override on cmd line):"
	@echo "  BUILD_DIR   = $(BUILD_DIR)"
	@echo "  BUILD_TYPE  = $(BUILD_TYPE)"
	@echo "  GENERATOR   = $(GENERATOR)"
	@echo "  JOBS        = $(JOBS)"
	@echo "  WEBGPU_IMPL = $(WEBGPU_IMPL)"
