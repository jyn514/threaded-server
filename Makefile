CC = clang

ifdef NDEBUG
# optimizations
override CFLAGS += -O3 -flto
else
# debug symbols
override CFLAGS += -g
endif

# warnings
override CFLAGS += -Wall -Wextra -Wpedantic -Wshadow

# libraries
override CFLAGS += -pthread

BUILD_DIR ?= build
PORT ?= 8080
SRC_DIR = src

VPATH = $(SRC_DIR)

MAKEFILE=$(firstword $(MAKEFILE_LIST))
ROOT=$(realpath $(dir $(MAKEFILE)))

.PHONY: all
all: $(BUILD_DIR)/main

run: all
	$(BUILD_DIR)/main $(PORT)

valgrind: all
	valgrind --leak-check=full $(BUILD_DIR)/main $(PORT)

.PHONY: test-minimal
test-minimal: test.bats test.sh
	./test.sh "$(MAKE)"

.PHONY: test
test: test-minimal
	clang-tidy --warnings-as-errors='*' src/*.c
	cppcheck --enable=all --error-exitcode=2 src/*.c

$(BUILD_DIR)/main: $(addprefix $(BUILD_DIR)/,main.o response.o parse.o dict.o)
	$(CC) -o $@ $^ $(CFLAGS) -lmagic

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -c -o $@

$(BUILD_DIR)/main.o: response.h
$(BUILD_DIR)/response.o: response.h parse.h dict.h
$(BUILD_DIR)/parse.o: parse.h dict.h
$(BUILD_DIR)/dict.o: dict.h

.PHONY: clean
clean:
	$(RM) $(BUILD_DIR)/*.o $(BUILD_DIR)/main
