CC = clang
CXX = clang++

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
override CFLAGS += -pthread -I.

override CXXFLAGS += $(CFLAGS)

BUILD_DIR = build
SRC_DIR = src

VPATH = $(SRC_DIR)

MAKEFILE=$(firstword $(MAKEFILE_LIST))
ROOT=$(realpath $(dir $(MAKEFILE)))

.PHONY: all
all: $(BUILD_DIR)/main

$(BUILD_DIR)/main: $(addprefix $(BUILD_DIR)/,main.o response.o parse.o) lib/libmagic.so
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(BUILD_DIR)/main.o: response.h
$(BUILD_DIR)/response.o: response.h parse.h
$(BUILD_DIR)/parse.o: parse.h

.PHONY: clean
clean:
	$(RM) $(BUILD_DIR)/*.o $(BUILD_DIR)/main
