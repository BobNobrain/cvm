MODE ?= debug

CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c11
CPPFLAGS = -I"./header"
LD_FLAGS = -lm

ifeq ($(MODE),release)
	CFLAGS += -O3 -DNDEBUG
else ifeq ($(MODE),debug)
	CFLAGS += -DDEBUG -g
else
	$(error Unknown MODE="$(MODE)". Only debug/release supported)
endif

OUT_DIR = out/$(MODE)
OUT_DIR_LIBS = out/$(MODE)/libs

HEADERS := $(wildcard header/*.h)
SRC_LIBS := $(wildcard src/*/)
ALL_LIB_NAMES := $(patsubst src/%/,%,$(SRC_LIBS))
SRC_EXECS := $(wildcard src/*.c)
ALL_EXEC_NAMES := $(patsubst src/%.c,%,$(SRC_EXECS))

CPPFLAGS += $(patsubst %,-I"./src/%",$(ALL_LIB_NAMES))

.PHONY: all clean test $(ALL_EXEC_NAMES)

all: clean $(ALL_EXEC_NAMES)

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
	$(CC) $$^ $(LD_FLAGS) -o $$@

$(OUT_DIR)/$(1).o: src/$(1).c $(foreach lib,$(2),src/$(lib)/$(lib).h)
	@echo "Compiling executable '$1'..."
	@echo
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $$< -o $$@
endef

$(eval $(call EXEC_COMPILATION_RULE,exprc,$(ALL_LIB_NAMES)))
$(eval $(call EXEC_COMPILATION_RULE,c,$(ALL_LIB_NAMES)))
$(eval $(call EXEC_COMPILATION_RULE,vm,$(ALL_LIB_NAMES)))

clean:
	rm -rf out
	mkdir -p out/debug/libs
	mkdir -p out/release/libs

.SECONDEXPANSION:

# internal lib object files
$(OUT_DIR_LIBS)/%.o: $$(wildcard src/$$*/*.c) $$(wildcard src/$$*/*.h)
	@echo "Compiling library: '$*'..."
	@echo "  from: $(filter %.c,$^)"
	@echo
	$(CC) $(CPPFLAGS) $(CFLAGS) -r $(filter %.c,$^) -o $@

test:
	@echo "No tests yet"
