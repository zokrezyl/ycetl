BUILD_DIR  ?= build-linux
BUILD_TYPE ?= Release
GENERATOR  ?= Ninja
JOBS       ?= $(shell nproc)

CMAKE      ?= cmake

# Bare `make` lists targets rather than building — run `make all` (or `build`)
# to actually compile.
.DEFAULT_GOAL := help

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

# ---------------------------------------------------------------------------
# WebGPU bindings example (examples/webgpu) — explicit, separated build stages.
#
# Two implementations, each in its OWN build tree (build-linux/webgpu-<impl>)
# so they never clobber each other. Every stage is a REAL FILE TARGET, chained
# by file prerequisites — so invoking a later stage builds the earlier ones
# first, and a stage only re-runs when its own inputs changed:
#
#   examples/webgpu/<impl>/include/webgpu.h
#     ─stage─ configure   → build-linux/webgpu-<impl>/CMakeCache.txt
#     ─stage─ tool        → .../examples/webgpu/webgpu_introspect   (the emitter)
#     ─stage─ gen         → .../webgpu_ctypes.py  +  .../webgpu_tree.hpp
#     ─stage─ tree-check  → .../examples/webgpu/webgpu_tree_check    (C++ consumer)
#     ─stage─ test        → ctest
#
# Targets:  webgpu-<impl>-{configure,tool,gen,tree-check,test,show,clean}
#           webgpu-<impl>            (= tree-check: builds the whole example)
# Each tree is trimmed to the example only (no unit_test / yce).
# ---------------------------------------------------------------------------

WEBGPU_SRC := examples/webgpu
WEBGPU_CFG := -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DYCETL_BUILD_WEBGPU=ON \
              -DYCETL_BUILD_TESTS=OFF -DYCETL_BUILD_EXAMPLES=OFF

# ===== dawn ================================================================
DAWN     := $(BUILD_DIR)/webgpu-dawn
DAWN_OUT := $(DAWN)/examples/webgpu

# stage: configure
$(DAWN)/CMakeCache.txt: $(WEBGPU_SRC)/CMakeLists.txt
	$(CMAKE) -S . -B $(DAWN) -G "$(GENERATOR)" -DWEBGPU_IMPL=dawn $(WEBGPU_CFG)
# stage: tool  (the libclang parser/emitter binary)
$(DAWN_OUT)/webgpu_introspect: $(WEBGPU_SRC)/webgpu_introspect.cpp $(DAWN)/CMakeCache.txt
	$(CMAKE) --build $(DAWN) --parallel $(JOBS) --target webgpu_introspect
# stage: gen  (run the tool → emit the .py and .hpp)
$(DAWN_OUT)/webgpu_tree.hpp: $(DAWN_OUT)/webgpu_introspect $(WEBGPU_SRC)/dawn/include/webgpu.h
	$(CMAKE) --build $(DAWN) --parallel $(JOBS) --target webgpu_outputs
# stage: tree-check  (the C++ constexpr consumer binary)
$(DAWN_OUT)/webgpu_tree_check: $(DAWN_OUT)/webgpu_tree.hpp $(WEBGPU_SRC)/webgpu_tree_check.cpp
	$(CMAKE) --build $(DAWN) --parallel $(JOBS) --target webgpu_tree_check
# stage: emit  (ycetl walks the tree at constexpr → rebuilds the .py)
$(DAWN_OUT)/webgpu_ctypes_from_tree.py: $(DAWN_OUT)/webgpu_tree.hpp $(WEBGPU_SRC)/webgpu_emit_py.cpp
	$(CMAKE) --build $(DAWN) --parallel $(JOBS) --target webgpu_from_tree

.PHONY: webgpu-dawn-configure webgpu-dawn-tool webgpu-dawn-gen \
        webgpu-dawn-tree-check webgpu-dawn-emit webgpu-dawn-verify \
        webgpu-dawn webgpu-dawn-test webgpu-dawn-show webgpu-dawn-clean
webgpu-dawn-configure:  $(DAWN)/CMakeCache.txt
webgpu-dawn-tool:       $(DAWN_OUT)/webgpu_introspect
webgpu-dawn-gen:        $(DAWN_OUT)/webgpu_tree.hpp
webgpu-dawn-tree-check: $(DAWN_OUT)/webgpu_tree_check
webgpu-dawn-emit:       $(DAWN_OUT)/webgpu_ctypes_from_tree.py
webgpu-dawn:            $(DAWN_OUT)/webgpu_tree_check
webgpu-dawn-test: $(DAWN_OUT)/webgpu_tree_check
	ctest --test-dir $(DAWN) -R webgpu --output-on-failure
# verify: the two paths (direct libclang vs ycetl-from-tree) are byte-identical
webgpu-dawn-verify: $(DAWN_OUT)/webgpu_ctypes_from_tree.py
	@cmp -s $(DAWN_OUT)/webgpu_ctypes.py $(DAWN_OUT)/webgpu_ctypes_from_tree.py \
	  && echo "dawn: IDENTICAL — ycetl(tree) == direct(libclang)" \
	  || { echo "dawn: DIFFER"; diff $(DAWN_OUT)/webgpu_ctypes.py $(DAWN_OUT)/webgpu_ctypes_from_tree.py | head -20; exit 1; }
webgpu-dawn-show: $(DAWN_OUT)/webgpu_tree.hpp
	@head -n 40 $(DAWN_OUT)/webgpu_ctypes.py
webgpu-dawn-clean:
	rm -f $(DAWN_OUT)/webgpu_ctypes.py $(DAWN_OUT)/webgpu_tree.hpp $(DAWN_OUT)/webgpu_ctypes_from_tree.py

# ===== native ==============================================================
NATIVE     := $(BUILD_DIR)/webgpu-native
NATIVE_OUT := $(NATIVE)/examples/webgpu

# stage: configure
$(NATIVE)/CMakeCache.txt: $(WEBGPU_SRC)/CMakeLists.txt
	$(CMAKE) -S . -B $(NATIVE) -G "$(GENERATOR)" -DWEBGPU_IMPL=native $(WEBGPU_CFG)
# stage: tool
$(NATIVE_OUT)/webgpu_introspect: $(WEBGPU_SRC)/webgpu_introspect.cpp $(NATIVE)/CMakeCache.txt
	$(CMAKE) --build $(NATIVE) --parallel $(JOBS) --target webgpu_introspect
# stage: gen
$(NATIVE_OUT)/webgpu_tree.hpp: $(NATIVE_OUT)/webgpu_introspect $(WEBGPU_SRC)/native/include/webgpu.h
	$(CMAKE) --build $(NATIVE) --parallel $(JOBS) --target webgpu_outputs
# stage: tree-check
$(NATIVE_OUT)/webgpu_tree_check: $(NATIVE_OUT)/webgpu_tree.hpp $(WEBGPU_SRC)/webgpu_tree_check.cpp
	$(CMAKE) --build $(NATIVE) --parallel $(JOBS) --target webgpu_tree_check
# stage: emit  (ycetl walks the tree at constexpr → rebuilds the .py)
$(NATIVE_OUT)/webgpu_ctypes_from_tree.py: $(NATIVE_OUT)/webgpu_tree.hpp $(WEBGPU_SRC)/webgpu_emit_py.cpp
	$(CMAKE) --build $(NATIVE) --parallel $(JOBS) --target webgpu_from_tree

.PHONY: webgpu-native-configure webgpu-native-tool webgpu-native-gen \
        webgpu-native-tree-check webgpu-native-emit webgpu-native-verify \
        webgpu-native webgpu-native-test webgpu-native-show webgpu-native-clean
webgpu-native-configure:  $(NATIVE)/CMakeCache.txt
webgpu-native-tool:       $(NATIVE_OUT)/webgpu_introspect
webgpu-native-gen:        $(NATIVE_OUT)/webgpu_tree.hpp
webgpu-native-tree-check: $(NATIVE_OUT)/webgpu_tree_check
webgpu-native-emit:       $(NATIVE_OUT)/webgpu_ctypes_from_tree.py
webgpu-native:            $(NATIVE_OUT)/webgpu_tree_check
webgpu-native-test: $(NATIVE_OUT)/webgpu_tree_check
	ctest --test-dir $(NATIVE) -R webgpu --output-on-failure
# verify: the two paths (direct libclang vs ycetl-from-tree) are byte-identical
webgpu-native-verify: $(NATIVE_OUT)/webgpu_ctypes_from_tree.py
	@cmp -s $(NATIVE_OUT)/webgpu_ctypes.py $(NATIVE_OUT)/webgpu_ctypes_from_tree.py \
	  && echo "native: IDENTICAL — ycetl(tree) == direct(libclang)" \
	  || { echo "native: DIFFER"; diff $(NATIVE_OUT)/webgpu_ctypes.py $(NATIVE_OUT)/webgpu_ctypes_from_tree.py | head -20; exit 1; }
webgpu-native-show: $(NATIVE_OUT)/webgpu_tree.hpp
	@head -n 40 $(NATIVE_OUT)/webgpu_ctypes.py
webgpu-native-clean:
	rm -f $(NATIVE_OUT)/webgpu_ctypes.py $(NATIVE_OUT)/webgpu_tree.hpp $(NATIVE_OUT)/webgpu_ctypes_from_tree.py

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
	@echo "WebGPU example (examples/webgpu) — separated build stages, one target"
	@echo "each, chained by file deps (a later stage builds the earlier ones first)."
	@echo "Two impls, each in its own tree build-linux/webgpu-<impl>:"
	@echo ""
	@echo "  stage      target                    emits"
	@echo "  configure  webgpu-<impl>-configure    CMakeCache.txt"
	@echo "  tool       webgpu-<impl>-tool         webgpu_introspect (the emitter)"
	@echo "  gen        webgpu-<impl>-gen          webgpu_ctypes.py + webgpu_tree.hpp"
	@echo "  tree-check webgpu-<impl>-tree-check   webgpu_tree_check (C++ consumer)"
	@echo "  emit       webgpu-<impl>-emit         webgpu_ctypes_from_tree.py (ycetl@constexpr)"
	@echo "  verify     webgpu-<impl>-verify       diff: ycetl(tree) == direct(libclang)"
	@echo "  test       webgpu-<impl>-test         runs ctest"
	@echo "  (whole)    webgpu-<impl>              = tree-check"
	@echo "  show/clean webgpu-<impl>-{show,clean} peek at / remove generated files"
	@echo ""
	@echo "  <impl> = dawn | native.  e.g.  make webgpu-dawn-gen   make webgpu-native-test"
	@echo ""
	@echo "Variables (override on cmd line):"
	@echo "  BUILD_DIR   = $(BUILD_DIR)"
	@echo "  BUILD_TYPE  = $(BUILD_TYPE)"
	@echo "  GENERATOR   = $(GENERATOR)"
	@echo "  JOBS        = $(JOBS)"
