# Compiler
CC = gcc

QUIET = @

# Compiler flags
CFLAGS = --std=c17 -D_XOPEN_SOURCE=700 -O3 -W -Wall -Wextra -MMD -MP
PEDANTIC_FLAGS = -Werror -Wpedantic -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wswitch-default -Wcast-align=strict -Wbad-function-cast -Wstrict-overflow=4 -Winline -Wundef -Wnested-externs -Wunreachable-code -Wlogical-op -Wfloat-equal -Wredundant-decls -Wold-style-definition -Wwrite-strings -Wformat=2 -Wconversion -Wno-error=unused-parameter -Wno-error=inline -Wno-error=unreachable-code -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=missing-prototypes
SANITIZE_FLAGS = -fanalyzer -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize=integer-divide-by-zero -fsanitize=null -fsanitize=signed-integer-overflow -fsanitize=bounds-strict -fsanitize=alignment -fsanitize=object-size -pg
# PROFILE_FLAGS = -p

RELEASE_FLAGS = $(CFLAGS) -O3 -DNDEBUG
DEBUG_FLAGS = $(CFLAGS) $(PEDANTIC_FLAGS) $(SANITIZE_FLAGS)



BUILD_DIR = build
RELEASE_DIR = $(BUILD_DIR)/release
DEBUG_DIR = $(BUILD_DIR)/debug

DEBUG_EXECUTABLE = $(DEBUG_DIR)/heuristic_solver
RELEASE_EXECUTABLE = $(RELEASE_DIR)/heuristic_solver



# Source files
SRCS = graph.c reduction.c pqueue.c greedy.c dynamic_array.c heuristic_solver.c

# Object files
RELEASE_OBJS = $(addprefix $(RELEASE_DIR)/obj/,$(SRCS:.c=.o))
DEBUG_OBJS = $(addprefix $(DEBUG_DIR)/obj/,$(SRCS:.c=.o))

# Dependency files
RELEASE_DEPS = $(RELEASE_OBJS:.o=.d)
DEBUG_DEPS = $(DEBUG_OBJS:.o=.d)



# first target ==> default
release: $(RELEASE_EXECUTABLE)
all: release
debug: $(DEBUG_EXECUTABLE)
pedantic: CFLAGS += $(PEDANTIC_FLAGS)
pedantic: $(RELEASE_EXECUTABLE)



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
	$(QUIET)rm -f $(DEBUG_OBJS) $(RELEASE_OBJS) $(DEBUG_DEPS) $(RELEASE_DEPS) $(DEBUG_EXECUTABLE) $(RELEASE_EXECUTABLE)
	$(QUIET)rmdir --ignore-fail-on-non-empty $(RELEASE_DIR)/obj $(DEBUG_DIR)/obj 2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(RELEASE_DIR) $(DEBUG_DIR) 2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(BUILD_DIR) 2>/dev/null || true



# Include auto-generated dependency files
-include $(RELEASE_DEPS) $(DEBUG_DEPS)

.PHONY: clean pedantic release debug
