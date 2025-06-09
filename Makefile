# Compiler
CC = gcc

QUIET = @ # remove this @ for verbose output

# Compiler flags
CFLAGS = --std=c17 -D_XOPEN_SOURCE=700 -O3 -W -Wall -Wextra -MMD -MP
PEDANTIC_FLAGS = -Werror -Wpedantic -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wswitch-default -Wcast-align=strict -Wbad-function-cast -Wstrict-overflow=4 -Winline -Wundef -Wnested-externs -Wunreachable-code -Wlogical-op -Wfloat-equal -Wredundant-decls -Wold-style-definition -Wwrite-strings -Wformat=2 -Wconversion -Wno-error=unused-parameter -Wno-error=inline -Wno-error=unreachable-code -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=missing-prototypes
SANITIZE_FLAGS = -fanalyzer -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize=integer-divide-by-zero -fsanitize=null -fsanitize=signed-integer-overflow -fsanitize=bounds-strict -fsanitize=alignment -fsanitize=object-size -pg

RELEASE_FLAGS = $(CFLAGS) -O3 -DNDEBUG
STRICT_FLAGS = $(RELEASE_FLAGS) $(PEDANTIC_FLAGS)
DEBUG_FLAGS = $(CFLAGS) $(PEDANTIC_FLAGS) $(SANITIZE_FLAGS)



BUILD_DIR = build
RELEASE_DIR = $(BUILD_DIR)/release
STRICT_DIR = $(BUILD_DIR)/strict
DEBUG_DIR = $(BUILD_DIR)/debug

RELEASE_EXECUTABLE = $(RELEASE_DIR)/heuristic_solver
STRICT_EXECUTABLE = $(STRICT_DIR)/heuristic_solver
DEBUG_EXECUTABLE = $(DEBUG_DIR)/heuristic_solver



# Source files
SRCS = graph.c reduction.c pqueue.c greedy.c dynamic_array.c heuristic_solver.c

# Object files
RELEASE_OBJS = $(addprefix $(RELEASE_DIR)/obj/,$(SRCS:.c=.o))
STRICT_OBJS = $(addprefix $(STRICT_DIR)/obj/,$(SRCS:.c=.o))
DEBUG_OBJS = $(addprefix $(DEBUG_DIR)/obj/,$(SRCS:.c=.o))

# Dependency files
RELEASE_DEPS = $(RELEASE_OBJS:.o=.d)
STRICT_DEPS = $(STRICT_OBJS:.o=.d)
DEBUG_DEPS = $(DEBUG_OBJS:.o=.d)



# first target ==> default
release: $(RELEASE_EXECUTABLE)
strict: $(STRICT_EXECUTABLE)
debug: $(DEBUG_EXECUTABLE)
all: release



$(RELEASE_EXECUTABLE): $(RELEASE_OBJS)
	@echo Linking...
	$(QUIET)$(CC) $(RELEASE_FLAGS) -o $(RELEASE_EXECUTABLE) $(RELEASE_OBJS)

# compile C source files into object files
$(RELEASE_DIR)/obj/%.o: %.c | $(RELEASE_DIR)
	@echo Compiling $<
	$(QUIET)$(CC) $(RELEASE_FLAGS) -c $< -o $@

# Ensure directory exists
$(RELEASE_DIR):
	$(QUIET)mkdir -p $(RELEASE_DIR)/obj

$(STRICT_EXECUTABLE): $(STRICT_OBJS)
	@echo Linking...
	$(QUIET)$(CC) $(STRICT_FLAGS) -o $(STRICT_EXECUTABLE) $(STRICT_OBJS)

# compile C source files into object files
$(STRICT_DIR)/obj/%.o: %.c | $(STRICT_DIR)
	@echo Compiling $<
	$(QUIET)$(CC) $(STRICT_FLAGS) -c $< -o $@

# Ensure directory exists
$(STRICT_DIR):
	$(QUIET)mkdir -p $(STRICT_DIR)/obj

$(DEBUG_EXECUTABLE): $(DEBUG_OBJS)
	@echo Linking...
	$(QUIET)$(CC) $(DEBUG_FLAGS) -o $(DEBUG_EXECUTABLE) $(DEBUG_OBJS)

# compile C source files into object files
$(DEBUG_DIR)/obj/%.o: %.c | $(DEBUG_DIR)
	@echo Compiling $<
	$(QUIET)$(CC) $(DEBUG_FLAGS) -c $< -o $@

# Ensure directory exists
$(DEBUG_DIR):
	$(QUIET)mkdir -p $(DEBUG_DIR)/obj

# Clean up the build files
clean:
	$(QUIET)rm -f $(RELEASE_OBJS) $(STRICT_OBJS) $(DEBUG_OBJS) $(RELEASE_DEPS) $(STRICT_DEPS) $(DEBUG_DEPS) $(RELEASE_EXECUTABLE) $(STRICT_EXECUTABLE) $(DEBUG_EXECUTABLE)
	$(QUIET)rmdir --ignore-fail-on-non-empty $(RELEASE_DIR)/obj $(STRICT_DIR)/obj $(DEBUG_DIR)/obj 2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(RELEASE_DIR) $(STRICT_DIR) $(DEBUG_DIR) 2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(BUILD_DIR) 2>/dev/null || true



# Include auto-generated dependency files
-include $(RELEASE_DEPS) $(STRICT_DEPS) $(DEBUG_DEPS)

.PHONY: release strict debug clean
