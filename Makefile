# TODO: improve this Makefile


# Compiler
CC = gcc

# Compiler flags
CFLAGS = --std=c17 -O3 -W -Wall -Wextra
PEDANTIC_FLAGS = -Werror -Wpedantic -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wswitch-default -Wcast-align=strict -Wbad-function-cast -Wstrict-overflow=4 -Winline -Wundef -Wnested-externs -Wunreachable-code -Wlogical-op -Wfloat-equal -Wredundant-decls -Wold-style-definition -Wwrite-strings -Wformat=2 -Wconversion -Wno-error=unused-parameter -Wno-error=inline -Wno-error=unreachable-code -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=missing-prototypes
SANITIZE_FLAGS = -fanalyzer -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize=integer-divide-by-zero -fsanitize=null -fsanitize=signed-integer-overflow -fsanitize=bounds-strict -fsanitize=alignment -fsanitize=object-size -pg
RELEASE_FLAGS = -O3 -DNDEBUG
PROFILE_FLAGS = -p



# The name of the build directory
BUILD_DIR = build

# Target executable
TARGET = $(BUILD_DIR)/main

# Source files
SRCS = graph.c reduction.c pqueue.c greedy.c main.c

# Header files
HDRS = graph.h reduction.h greedy.h pqueue.h

# Object files
OBJS = $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))




# Default target
release: CFLAGS += $(RELEASE_FLAGS)
release: clean all

all: $(TARGET)

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Rule to link object files and create the executable
$(TARGET): $(OBJS)
	@echo Linking $(TARGET)
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile C source files into object files
$(BUILD_DIR)/%.o: %.c $(HDRS) | $(BUILD_DIR)
	@echo Compiling $<
	@$(CC) $(CFLAGS) -c $< -o $@

# Use pedantic flags to enforce quality standards
pedantic: CFLAGS += $(PEDANTIC_FLAGS)
pedantic: clean all

sanitize: PEDANTIC_FLAGS += $(SANITIZE_FLAGS)
sanitize: pedantic

profile: RELEASE_FLAGS += $(PROFILE_FLAGS)
profile: release

# Clean up the build files
clean:
	rm -rf $(OBJS) $(TARGET)
	@rmdir --ignore-fail-on-non-empty $(BUILD_DIR) 2>/dev/null || true

.PHONY: all clean pedantic sanitize release profile
