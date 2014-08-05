# build.mk - set (some of) the following variables, and include me.
# SRCS:       : source files
# TARGET:     : target filename
# TARGET_TYPE : either 'executable' or 'static_library'
# DEPENDENCIES: sibling directories that we depend on, all of which
#               should have TARGET_TYPE = static_library.
#               The directories are added to include path, and
#               and the default targets are statically linked in.
############################################################
MAKEFLAGS += -rR
.SUFFIXES:
ROOT_OBJDIR := out

COMPILER ?= clang
COMPILER_gcc   := g++-4.8
COMPILER_clang := clang++-3.4
CXX = $(COMPILER_$(COMPILER))

ifneq ($(shell $(CXX) -v 2>/dev/null && echo ok), ok)
    COMPILER := gcc
endif

CF_STANDARD := -std=c++0x
CF_DEBUG    := -g3 -gdwarf-2
CF_OPTIMIZE := -O0
CF_WARNINGS := -Wall -Wextra -Woverloaded-virtual -Werror -Wno-unused-parameter -Wno-unknown-pragmas
############################################################
INCLUDES := $(patsubst %, -I../%/, $(DEPENDENCIES))
LIBS := $(foreach subdir, $(DEPENDENCIES), ../$(subdir)/$(shell cd ../$(subdir); make get-target))
CFG ?= dbg

ifeq ($(CFG), dbg)
else ifeq ($(CFG), opt)
    CF_OPTIMIZE := -O3
else ifeq ($(CFG), cov)
    COMPILER := gcc
    CFLAGS  += -fprofile-arcs -ftest-coverage -DCONFIG_NO_PARSER_TRACE
    LDFLAGS += -fprofile-arcs -ftest-coverage
else ifeq ($(CFG), prof)
    COMPILER := gcc
    CF_OPTIMIZE := -O1
    CFLAGS  += -pg -DCONFIG_NO_PARSER_TRACE -DCONFIG_PROFILING
    LDFLAGS += -pg
else
    $(error Invalid CFG: '$(CFG)')
endif

OBJDIR 	   := $(ROOT_OBJDIR)/$(CFG)
TARGET_OBJ := $(OBJDIR)/$(TARGET)-$(CFG)
CFGFILE    := $(ROOT_OBJDIR)/$(TARGET)-$(CFG).flag

$(TARGET): $(TARGET_OBJ) $(CFGFILE)
	cp $(TARGET_OBJ) $(TARGET)

# This will force a relink when the CFG has changed from the last build
$(ROOT_OBJDIR)/$(TARGET)-%.flag:
	@mkdir -p $(ROOT_OBJDIR)
	@rm -f $(ROOT_OBJDIR)/$(TARGET)-*.flag
	@touch $@

ACTUAL_CFLAGS := -I. $(INCLUDES) $(CF_STANDARD) $(CF_DEBUG) $(CF_OPTIMIZE) $(CF_WARNINGS) $(CFLAGS)
ACTUAL_LDFLAGS := $(LDFLAGS)

SRCS := $(patsubst ./%, %, $(SRCS))
OBJS := $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRCS))
all: $(TARGET)

ifeq (static_library, $(TARGET_TYPE))
$(TARGET_OBJ): $(OBJS) $(FLAGFILE)
	ar rcs $@ $(LDFLAGS) $^
else
$(TARGET_OBJ): $(OBJS) $(LIBS) $(FLAGFILE)
	@$(CXX) -o $@ $(LDFLAGS) $^
	@echo $(CXX) [$(CFG)] -o $@
endif

# Compiling
-include $(OBJS:.o=.d)

$(OBJDIR)/%.o: %.cpp | generated-files
	@mkdir -p $(OBJDIR)/`dirname $*`
	@$(CXX) $< -o $(OBJDIR)/$*.o $(ACTUAL_CFLAGS) -c -MMD -MP
	@echo $(CXX) [$(CFG)] $<

# Library dependencies
$(LIBS): $(addprefix ../, $(DEPENDENCIES))
	for subdir in $<; do $(MAKE) -C $<; done
.PHONY: $(addprefix ../, $(DEPENDENCIES))

# Misc. targets
.PHONY: clean allclean get-target generated-files
clean:
	-rm -rf $(TARGET) $(ROOT_OBJDIR)

allclean: clean
	for subdir in $(DEPENDENCIES); do $(MAKE) -C ../$$subdir clean; done

get-target:
	@echo $(TARGET)

generated-files:
