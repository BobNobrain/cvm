MODE ?= debug

CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c11
CPPFLAGS = -I"./header"
LD_FLAGS = -lm

ifeq ($(MODE),release)
	CFLAGS += -O3 -DNDEBUG
else ifeq ($(MODE),debug)
	CFLAGS += -DDEBUG -g -O0 -fsanitize=address
	LD_FLAGS += -g -fsanitize=address
else
	$(error Unknown MODE="$(MODE)". Only debug/release supported)
endif

OUT_DIR = out/$(MODE)
OUT_DIR_LIBS = out/$(MODE)/libs

EXT_HEADERS := $(wildcard header/*.h)

SRC_LIBS := $(wildcard src/*/)
ALL_LIB_NAMES := $(patsubst src/%/,%,$(SRC_LIBS))

SRC_EXECS := $(wildcard src/*.c)
ALL_EXEC_NAMES := $(patsubst src/%.c,%,$(SRC_EXECS))

LIB_HEADERS := $(foreach lib,$(ALL_LIB_NAMES),src/$(lib)/$(lib).h)

HEADERS := $(EXT_HEADERS) $(LIB_HEADERS)


CPPFLAGS += $(patsubst %,-I"./src/%",$(ALL_LIB_NAMES))

.PHONY: all clean setup test $(ALL_EXEC_NAMES)

all: clean $(ALL_EXEC_NAMES)

clean:
	rm -rf out

# A macro with rules to compile an executable:
# - a phony rule that translates executable name into the actual file name to build
# - a rule that links the object files into the executable
# - a rule that creates the object file for the executable
define EXEC_COMPILATION_RULE
$(1): $(OUT_DIR)/$(1)
	@echo "Succesfully compiled $$<"

$(OUT_DIR)/$(1): $(OUT_DIR)/$(1).o $(patsubst %,$(OUT_DIR_LIBS)/%.o,$(2))
	@echo "Linking '$$@' with following libs: $(2)..."
	@echo
	mkdir -p $$(@D)
	$$(CC) $$^ $$(LD_FLAGS) -o $$@

$(OUT_DIR)/$(1).o: src/$(1).c $(foreach lib,$(2),src/$(lib)/$(lib).h)
	@echo "Compiling executable '$1'..."
	@echo
	mkdir -p $$(@D)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -c $$< -o $$@
endef

$(eval $(call EXEC_COMPILATION_RULE,exprc,util lang ct))
$(eval $(call EXEC_COMPILATION_RULE,c,util lang ct))
$(eval $(call EXEC_COMPILATION_RULE,vm,util lang rt))

define LIB_COMPILATION_RULE
$(1)_C_FILES = $$(wildcard src/$(1)/*.c)
$(1)_O_FILES = $$(patsubst src/$(1)/%.c,$(OUT_DIR_LIBS)/$(1)/%.o,$$($(1)_C_FILES))
$(1)_H_FILES = $$(wildcard src/$(1)/*.h)

$(OUT_DIR_LIBS)/$(1).o: $$($(1)_O_FILES)
	@echo "Linking library: '$1'..."
	@echo "  from: $$($(1)_O_FILES)"
	@echo
	mkdir -p $$(@D)
	$$(CC) $$^ -r -o $$@

$(OUT_DIR_LIBS)/$(1)/%.o: src/$(1)/%.c $$($(1)_H_FILES) $$(HEADERS)
	@echo "Compiling library: '$1/$$<'..."
	@echo "  from: $$($(1)_H_FILES) $$(HEADERS)"
	@echo
	mkdir -p $$(@D)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -c $$< -o $$@
endef

$(foreach lib,$(ALL_LIB_NAMES),$(eval $(call LIB_COMPILATION_RULE,$(lib))))
# $(foreach lib,$(ALL_LIB_NAMES),$(info $(call LIB_COMPILATION_RULE,$(lib))))

# .SECONDEXPANSION:

# internal lib object files
# $(OUT_DIR_LIBS)/%.o: $$(wildcard src/$$*/*.c) $$(wildcard src/$$*/*.h)
# 	@echo "Compiling library: '$*'..."
# 	@echo "  from: $(filter %.c,$^)"
# 	@echo
# 	mkdir -p $(dir $@)
# 	$(CC) $(CPPFLAGS) $(CFLAGS) -r $(filter %.c,$^) -o $@

# $(OUT_DIR):
# 	mkdir -p $(OUT_DIR)

test:
	@echo "No tests yet"
