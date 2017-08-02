# $File: Makefile
# $Date: Tue Aug 01 17:54:37 2017 -0700
# Usage: PKG_CONFIG_PATH=installation/of/libraries/lib/pkgconfig make gen && make -j8

OBJ_DIR = obj
ENGINE_DIR = engine
CXX ?= g++
CC ?= gcc
PYTHON ?= python3
SHELL = bash

ifneq (,$(findstring clang,$(CXX)))
	OMP_FLAG=-fopenmp=libomp
else
	OMP_FLAG=-fopenmp
endif

PYTHON_CONFIG ?= python3-config

OPTFLAGS ?= -O3 -msse3
# OPTFLAGS ?= -g3 -fsanitize=address,undefined,leak -O2 -lasan
# OPTFLAGS ?= -g3 -fsanitize=leak -O2 -lubsan
DEFINES = -DDEBUG #-DUSE_PYBIND11

# generated files
GENERATE_DEF_FILES := ../engine/cmd.def ../engine/cmd_specific.def cmd_specific.def
GENERATED_FILES := ../engine/cmd.gen.h ../engine/cmd_specific.gen.h cmd_specific.gen.h

GIT_COMMIT_HASH = $(shell git rev-parse HEAD)
GIT_UNSTAGED = $(shell git diff-index --quiet HEAD -- && echo staged)

# optional extra packages
LIBS = zlib
INCLUDE_DIR += -I. $(shell pkg-config --cflags $(LIBS)) -isystem ../../vendor
INCLUDE_DIR += $(shell $(PYTHON_CONFIG) --cflags | cut -d ' ' -f 1)
LDFLAGS += $(shell pkg-config $(LIBS) --libs)
LDFLAGS += $(shell $(PYTHON_CONFIG) --ldflags)

CXXFLAGS ?=
CXXFLAGS += $(INCLUDE_DIR)
CXXFLAGS += -Wall -Wextra
#CXXFLAGS += -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare -Wno-unused-function -Wno-misleading-indentation -Wno-unused-but-set-variable
CXXFLAGS += -D GIT_COMMIT_HASH=${GIT_COMMIT_HASH} -D GIT_UNSTAGED=${GIT_UNSTAGED}
CXXFLAGS += $(DEFINES) -std=c++11 $(OPTFLAGS) -fPIC
#CXXFLAGS += -D_GLIBCXX_USE_CXX11_ABI=0	# for g++ on mac

LDFLAGS += $(OPTFLAGS)
LDFLAGS += -lm #-ltbb

# sources to include
ccSOURCES = $(shell find . -name "*.cc"  |  sed 's/^\.\///g')
# rts_engine
ccSOURCES += $(shell find ../engine -name "*.cc"  |  sed 's/^\.\///g')

# elf
ccSOURCES += $(shell find ../../elf/lib -name "*.cc"  |  sed 's/^\.\///g')

OBJS = $(addprefix $(OBJ_DIR)/,$(ccSOURCES:.cc=.o))
DEPFILES = $(OBJS:.o=.d)

SO = minirts.so

.PHONY: all clean run gen
.PRECIOUS: $(GENERATED_FILES) #$(PROTOFILES)

all: $(SO)

ifneq ($(MAKECMDGOALS), clean)
ifneq ($(MAKECMDGOALS), gen)
sinclude $(DEPFILES)
endif
endif

$(OBJ_DIR)/%.o: %.cc $(GENERATED_FILES) #$(PROTOFILES)
	@echo "[cc] $< ..."
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

$(OBJ_DIR)/%.d: %.cc Makefile
	@mkdir -pv $(dir $@)
	@echo "[dep] $< ..."
	@$(CXX) $(CXXFLAGS) -MM -MT "$(OBJ_DIR)/$(<:.cc=.o) $(OBJ_DIR)/$(<:.cc=.d)" "$<"  > "$@"

$(SO): $(OBJS)
	@echo "Linking so ..."
	@$(CXX) $^ -o $@ $(CXXFLAGS) $(LDFLAGS) -shared
	@echo "done."

gen: $(GENERATED_DEF_FILES)
	@echo "[inl] ..."
	$(PYTHON) ../engine/compile_cmds.py --def_file ../engine/cmd --name engine
	$(PYTHON) ../engine/compile_cmds.py --def_file ../engine/cmd_specific --name engine_specific
	$(PYTHON) ../engine/compile_cmds.py --def_file cmd_specific --name minirts_specific

clean:
	@rm -rvf $(OBJ_DIR) $(ENGINE_DIR) $(GENERATED_FILES)
