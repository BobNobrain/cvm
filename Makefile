MODE ?= debug

CC = gcc
CFLAGS = -Wall -Wextra -std=c11
CPPFLAGS = -I"./header"
LD_FLAGS = -lm
OUT_DIR = out/$(MODE)

HEADERS := $(wildcard header/*.h)

ifeq ($(MODE),release)
	CFLAGS += -O3 -DNDEBUG
else ifeq ($(MODE),debug)
	CFLAGS += -DDEBUG -g
else
	$(error Unknown MODE="$(MODE)". Only debug/release supported)
endif

.PHONY: all c exprc vm test clean

all: clean vm c

c: $(OUT_DIR)/c

exprc: $(OUT_DIR)/exprc

vm: $(OUT_DIR)/vm

clean:
	rm -rf out
	mkdir -p out/debug
	mkdir -p out/release

$(OUT_DIR)/c: $(OUT_DIR)/c.o
	$(CC) $^ $(LD_FLAGS) -o $@

$(OUT_DIR)/exprc: $(OUT_DIR)/exprc.o
	$(CC) $^ $(LD_FLAGS) -o $@

$(OUT_DIR)/vm: $(OUT_DIR)/vm.o
	$(CC) $^ $(LD_FLAGS) -o $@

$(OUT_DIR)/vm.o: src/vm.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OUT_DIR)/c.o: src/c.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OUT_DIR)/exprc.o: src/exprc.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test:
	echo "No tests yet"
