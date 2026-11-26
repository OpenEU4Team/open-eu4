PY ?= python3
CXX ?= g++
APP ?=
export APP

PROD_MAKEFILE := $(lastword $(MAKEFILE_LIST))
PROD_DIR := build/prod
PROD_FLAGS := -std=c++17 -O2 -g -Wall -Wextra -Wshadow -Werror -MMD -MP \
              -Wno-deprecated-declarations -fPIC
PROD_INC := -Isource/include -Isource -isystem source/vendor
PROD_PATCHER_FLAGS := $(PROD_FLAGS) -Ipatcher -I$(PROD_DIR)
prod_tus = $(foreach entry,$(wildcard $(1)/*),$(if $(filter vendor,$(notdir $(entry))),,$(if $(wildcard $(entry)/.),$(call prod_tus,$(entry)),$(filter %.cpp,$(entry)))))
PROD_TUS := $(sort $(call prod_tus,source))
PROD_OBJS := $(patsubst source/%.cpp,$(PROD_DIR)/%.o,$(PROD_TUS))
PATCHER_SRC := patcher/patcher.cpp
PATCHER_OBJ := $(PROD_DIR)/patcher/patcher.o
PROD_DSO := $(PROD_DIR)/libeu4_override.so
PROD_LINKER := patcher/linker_core.py

.DEFAULT_GOAL := prod
.PHONY: prod help release prod-force
prod: $(PROD_DSO)
	@printf 'prod: %s TUs + patcher -> %s\n' '$(words $(PROD_OBJS))' '$<'

help:
	@printf '%s\n' \
	    'make APP=/path/to/eu4                 Build the production carrier under build/prod/' \
	    'make release APP=/path/to/eu4 TAG=v1  Build and create a local annotated source tag' \
	    'Run from game directory with EU4_OVERRIDE_APP=/path/to/eu4 and' \
	    'LD_PRELOAD=/absolute/path/to/build/prod/libeu4_override.so ./eu4'

export TAG
release: prod
	@test -n "$$TAG" || { printf '%s\n' 'release requires TAG=<name>' >&2; exit 1; }
	@status=$$(git status --porcelain --untracked-files=no) && \
	    inputs=$$(git status --porcelain --untracked-files=all -- source patcher Makefile Makefile.prod) && \
	    test -z "$$status$$inputs" || { printf '%s\n' 'release requires committed source and a clean tracked tree and index' >&2; exit 1; }
	git tag -a -m "Release $$TAG" -- "$$TAG"

prod-force:

$(PROD_DIR)/corpus.o: $(PROD_OBJS) $(PROD_MAKEFILE) prod-force
	@mkdir -p $(@D)
	$(LD) -r -o $@.tmp $(PROD_OBJS)
	mv $@.tmp $@

$(PROD_DIR)/%.o: source/%.cpp $(PROD_MAKEFILE)
	@mkdir -p $(@D)
	$(CXX) $(PROD_FLAGS) $(PROD_INC) -c $< -o $@

$(PATCHER_OBJ): $(PATCHER_SRC) $(PROD_MAKEFILE)
	@mkdir -p $(@D)
	$(CXX) $(PROD_PATCHER_FLAGS) -c $< -o $@

$(PROD_DIR)/app_symbols.ld $(PROD_DIR)/app_symbols.json $(PROD_DIR)/override_exports.map $(PROD_DIR)/override_exports.dynlist &: $(PROD_DIR)/corpus.o $(PATCHER_OBJ) $(PROD_LINKER)
	$(PY) $(PROD_LINKER) --mode prod --app "$$APP" \
	    --patcher $(PATCHER_OBJ) --corpus $(PROD_DIR)/corpus.o \
	    --out-script $(PROD_DIR)/app_symbols.ld.tmp \
	    --out-version-script $(PROD_DIR)/override_exports.map.tmp \
	    --out-dynamic-list $(PROD_DIR)/override_exports.dynlist.tmp \
	    --out-report $(PROD_DIR)/app_symbols.json.tmp
	mv $(PROD_DIR)/app_symbols.ld.tmp $(PROD_DIR)/app_symbols.ld
	mv $(PROD_DIR)/override_exports.map.tmp $(PROD_DIR)/override_exports.map
	mv $(PROD_DIR)/override_exports.dynlist.tmp $(PROD_DIR)/override_exports.dynlist
	mv $(PROD_DIR)/app_symbols.json.tmp $(PROD_DIR)/app_symbols.json

$(PROD_DSO): $(PROD_DIR)/corpus.o $(PATCHER_OBJ) $(PROD_DIR)/override_exports.map $(PROD_DIR)/override_exports.dynlist $(PROD_DIR)/app_symbols.ld
	$(CXX) -shared -Wl,-z,defs,-z,now,-Bsymbolic-functions \
	    -Wl,--version-script=$(PROD_DIR)/override_exports.map \
	    -Wl,--dynamic-list=$(PROD_DIR)/override_exports.dynlist \
	    -Wl,-T,$(PROD_DIR)/app_symbols.ld \
	    -o $@.tmp $(PROD_DIR)/corpus.o $(PATCHER_OBJ)
	mv $@.tmp $@

-include $(PROD_OBJS:.o=.d) $(PATCHER_OBJ:.o=.d)
