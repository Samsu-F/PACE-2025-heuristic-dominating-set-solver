# Compiler
CC = gcc

QUIET = @ # remove this @ for verbose output

# Source files
SRCS = graph.c reduction.c pqueue.c greedy.c dynamic_array.c heuristic_solver.c


# Compiler flags
CFLAGS = --std=c17 -D_XOPEN_SOURCE=700 -W -Wall -Wextra -MMD -MP
PEDANTIC_FLAGS = -Werror -Wpedantic -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wswitch-default -Wcast-align=strict -Wbad-function-cast -Wstrict-overflow=4 -Winline -Wundef -Wnested-externs -Wunreachable-code -Wlogical-op -Wfloat-equal -Wredundant-decls -Wold-style-definition -Wwrite-strings -Wformat=2 -Wconversion -Wno-error=unused-parameter -Wno-error=inline -Wno-error=unreachable-code -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=missing-prototypes
SANITIZE_FLAGS = -fanalyzer -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize=integer-divide-by-zero -fsanitize=null -fsanitize=signed-integer-overflow -fsanitize=bounds-strict -fsanitize=alignment -fsanitize=object-size -g

CFLAGS_RELEASE = $(CFLAGS) -O3 -DNDEBUG
CFLAGS_STRICT  = $(CFLAGS_RELEASE) $(PEDANTIC_FLAGS)
CFLAGS_DEBUG   = $(CFLAGS) $(PEDANTIC_FLAGS) $(SANITIZE_FLAGS) -O3



BUILD_DIR = build
DIR_RELEASE = $(BUILD_DIR)/release
DIR_STRICT  = $(BUILD_DIR)/strict
DIR_DEBUG   = $(BUILD_DIR)/debug

TARGET_RELEASE = $(DIR_RELEASE)/heuristic_solver
TARGET_STRICT  = $(DIR_STRICT)/heuristic_solver
TARGET_DEBUG   = $(DIR_DEBUG)/heuristic_solver



# Object files
OBJS_RELEASE = $(SRCS:%.c=$(DIR_RELEASE)/obj/%.o)
OBJS_STRICT  = $(SRCS:%.c=$(DIR_STRICT)/obj/%.o)
OBJS_DEBUG   = $(SRCS:%.c=$(DIR_DEBUG)/obj/%.o)

# Dependency files
DEPS_RELEASE = $(OBJS_RELEASE:.o=.d)
DEPS_STRICT  = $(OBJS_STRICT:.o=.d)
DEPS_DEBUG   = $(OBJS_DEBUG:.o=.d)



# first target ==> default
release: $(TARGET_RELEASE)
strict: $(TARGET_STRICT)
debug: $(TARGET_DEBUG)
all: help


# avoid duplication for targets that only differ in the C flags used
define COMPILE_RULE
$(TARGET_$(1)): $(OBJS_$(1))
	@echo Linking $$@
	$$(QUIET)$$(CC) $$(CFLAGS_$(1)) -o $$@ $$(OBJS_$(1))

$(DIR_$(1))/obj/%.o: %.c | $(DIR_$(1))
	@echo Compiling $$<
	$$(QUIET)$$(CC) $$(CFLAGS_$(1)) -c $$< -o $$@
endef

$(eval $(call COMPILE_RULE,RELEASE))
$(eval $(call COMPILE_RULE,STRICT))
$(eval $(call COMPILE_RULE,DEBUG))



$(DIR_RELEASE) $(DIR_STRICT) $(DIR_DEBUG):
	$(QUIET)mkdir -p $@/obj


# Clean up the build files
clean:
	$(QUIET)rm -f $(OBJS_RELEASE) $(DEPS_RELEASE) $(TARGET_RELEASE)
	$(QUIET)rm -f $(OBJS_STRICT)  $(DEPS_STRICT)  $(TARGET_STRICT)
	$(QUIET)rm -f $(OBJS_DEBUG)   $(DEPS_DEBUG)   $(TARGET_DEBUG)
	$(QUIET)rmdir --ignore-fail-on-non-empty $(DIR_RELEASE)/obj $(DIR_RELEASE) 2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(DIR_STRICT)/obj  $(DIR_STRICT)  2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(DIR_DEBUG)/obj   $(DIR_DEBUG)   2>/dev/null || true
	$(QUIET)rmdir --ignore-fail-on-non-empty $(BUILD_DIR) 2>/dev/null || true


help:
	@echo "Available targets:"
	@echo "  make release     - Build release (default)"
	@echo "  make strict      - Build with pedantic warnings"
	@echo "  make debug       - Build debug with pedantic warnings and with sanitizers"
	@echo "  make clean       - Remove build artifacts"
	@echo "  make help        - print this help text"



# Include auto-generated dependency files
-include $(DEPS_RELEASE) $(DEPS_STRICT) $(DEPS_DEBUG)

.PHONY: release strict debug clean help all
