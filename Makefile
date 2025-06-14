# Compiler
CC = gcc

QUIET = @ # remove this @ for verbose output

# Source files
SRCS = graph.c reduction.c pqueue.c greedy.c dynamic_array.c heuristic_solver.c


# Compiler flags
CFLAGS = -std=c17 -D_XOPEN_SOURCE=700 -W -Wall -Wextra -MMD -MP
PEDANTIC_FLAGS = -Werror -Wpedantic -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wswitch-default -Wcast-align=strict -Wbad-function-cast -Wstrict-overflow=4 -Winline -Wundef -Wnested-externs -Wunreachable-code -Wlogical-op -Wfloat-equal -Wredundant-decls -Wold-style-definition -Wwrite-strings -Wformat=2 -Wconversion -Wno-error=unused-parameter -Wno-error=inline -Wno-error=unreachable-code -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=missing-prototypes
SANITIZE_FLAGS = -fanalyzer -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize=integer-divide-by-zero -fsanitize=null -fsanitize=signed-integer-overflow -fsanitize=bounds-strict -fsanitize=alignment -fsanitize=object-size -g
OPTIMIZATION_FLAGS = -Ofast -fno-signed-zeros -fipa-pta -fipa-reorder-for-locality

CFLAGS_RELEASE = $(CFLAGS) $(OPTIMIZATION_FLAGS) -DNDEBUG
CFLAGS_STRICT  = $(CFLAGS) $(OPTIMIZATION_FLAGS) -DNDEBUG $(PEDANTIC_FLAGS)
CFLAGS_LOG     = $(CFLAGS) $(OPTIMIZATION_FLAGS) -DNDEBUG $(PEDANTIC_FLAGS) -DDEBUG_LOG
CFLAGS_DEBUG   = $(CFLAGS) $(OPTIMIZATION_FLAGS) $(PEDANTIC_FLAGS) $(SANITIZE_FLAGS) -DDEBUG_LOG



BUILD_DIR = build
DIR_RELEASE = $(BUILD_DIR)/release
DIR_STRICT  = $(BUILD_DIR)/strict
DIR_LOG     = $(BUILD_DIR)/log
DIR_DEBUG   = $(BUILD_DIR)/debug

TARGET_RELEASE = $(DIR_RELEASE)/heuristic_solver
TARGET_STRICT  = $(DIR_STRICT)/heuristic_solver
TARGET_LOG     = $(DIR_LOG)/heuristic_solver
TARGET_DEBUG   = $(DIR_DEBUG)/heuristic_solver



# Object files
OBJS_RELEASE = $(SRCS:%.c=$(DIR_RELEASE)/obj/%.o)
OBJS_STRICT  = $(SRCS:%.c=$(DIR_STRICT)/obj/%.o)
OBJS_LOG     = $(SRCS:%.c=$(DIR_LOG)/obj/%.o)
OBJS_DEBUG   = $(SRCS:%.c=$(DIR_DEBUG)/obj/%.o)

# Dependency files
DEPS_RELEASE = $(OBJS_RELEASE:.o=.d)
DEPS_STRICT  = $(OBJS_STRICT:.o=.d)
DEPS_LOG     = $(OBJS_LOG:.o=.d)
DEPS_DEBUG   = $(OBJS_DEBUG:.o=.d)



# first target ==> default
release: $(TARGET_RELEASE)
strict: $(TARGET_STRICT)
log: $(TARGET_LOG)
debug: $(TARGET_DEBUG)
all: help


# avoid duplication for targets that only differ in the C flags used
define COMPILE_RULE
$(TARGET_$(1)): $(OBJS_$(1))
	@echo Linking $$@
	$$(QUIET)$$(CC) $$(CFLAGS_$(1)) -o $$@ $$(OBJS_$(1))
	@printf '\033[1;32mFinished successfully. The compiled executable can be found at %s\033[0m\n' '$$(TARGET_$(1))'

$(DIR_$(1))/obj/%.o: src/%.c | $(DIR_$(1))
	@echo Compiling $$<
	$$(QUIET)$$(CC) $$(CFLAGS_$(1)) -c $$< -o $$@
endef

$(eval $(call COMPILE_RULE,RELEASE))
$(eval $(call COMPILE_RULE,STRICT))
$(eval $(call COMPILE_RULE,LOG))
$(eval $(call COMPILE_RULE,DEBUG))



$(DIR_RELEASE) $(DIR_STRICT) $(DIR_LOG) $(DIR_DEBUG):
	$(QUIET)mkdir -p $@/obj


# Clean up the build files
clean:
	$(QUIET)rm -f $(OBJS_RELEASE) $(DEPS_RELEASE) $(TARGET_RELEASE)
	$(QUIET)rm -f $(OBJS_STRICT)  $(DEPS_STRICT)  $(TARGET_STRICT)
	$(QUIET)rm -f $(OBJS_LOG)     $(DEPS_LOG)     $(TARGET_LOG)
	$(QUIET)rm -f $(OBJS_DEBUG)   $(DEPS_DEBUG)   $(TARGET_DEBUG)
	$(QUIET)rmdir --ignore-fail-on-non-empty $(DIR_RELEASE)/obj $(DIR_RELEASE) 2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(DIR_STRICT)/obj  $(DIR_STRICT)  2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(DIR_LOG)/obj     $(DIR_LOG)     2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(DIR_DEBUG)/obj   $(DIR_DEBUG)   2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(BUILD_DIR) 2>/dev/null || true


help:
	@echo "Available targets:"
	@echo "  make release     - Build release (default)"
	@echo "  make strict      - Build with pedantic compiler warnings"
	@echo "  make log         - Same as strict but enable logging"
	@echo "  make debug       - Build debug with pedantic compiler warnings, assertions, sanitizers, and logging"
	@echo "  make clean       - Remove build artifacts"
	@echo "  make help        - print this help text"


# Include auto-generated dependency files
-include $(DEPS_RELEASE) $(DEPS_STRICT) $(DEPS_LOG) $(DEPS_DEBUG)

.PHONY: release strict log debug clean help all
