UNAME_S := $(shell uname -s)
PYTHON_CONFIG ?= python3-config
ifeq ($(UNAME_S),Linux)
	CXX ?= g++
endif
INCLUDE_DIR = -I ../vendor
INCLUDE_DIR += $(shell $(PYTHON_CONFIG) --cflags | cut -d ' ' -f 1)

ifneq (,$(findstring clang,$(CXX)))
	OMP_FLAG=-fopenmp=libomp
else
	OMP_FLAG=-fopenmp
endif

OPTFLAGS ?= -O3 -msse3 -march=native -pthread
#OPTFLAGS ?= -g3 -fsanitize=address,undefined -O0
DEFINES = -DUSE_PYBIND11 -DNDEBUG

# optional extra packages
LIBS = ale
INCLUDE_DIR += $(shell pkg-config --cflags $(LIBS))
LDFLAGS += $(shell pkg-config $(LIBS) --libs)
LDFLAGS += $(shell $(PYTHON_CONFIG) --ldflags) -lrt

CXXFLAGS += -fPIC
CXXFLAGS += $(INCLUDE_DIR) $(OMP_FLAG) -Wno-deprecated-declarations -Wall -Wextra -Wno-sign-compare
CXXFLAGS += $(DEFINES) -std=c++11 $(OPTFLAGS)

LDFLAGS += $(OPTFLAGS)
LDFLAGS += $(OMP_FLAG)
LDFLAGS += -lstdc++

GIT_COMMIT_HASH = $(shell git rev-parse HEAD)
GIT_UNSTAGED = $(shell git diff-index --quiet HEAD -- && echo staged)

SHELL = bash
OBJ_DIR = obj
ccSRCS = $(shell find . -name "*.cc" | sed 's/^\.\///g')
MAIN_SRCS := $(shell find -L -name "*.cpp" | cut -c 3- | grep -v '^_')

OBJS = $(addprefix $(OBJ_DIR)/,$(ccSRCS:.cc=.o))
MAIN_OBJ  := $(addprefix $(OBJ_DIR)/, $(MAIN_SRCS:.cpp=.o))
DEPS = $(OBJS:.o=.d) $(MAIN_OBJ:.o=.d)
BINS = $(MAIN_SRCS:.cpp=.bin)
SO = atari_game.so

.PHONY: all clean run

all: $(BINS) $(SO)

ifneq ($(MAKECMDGOALS), clean)
sinclude $(DEPS)
endif

$(SO): $(OBJS)
	@echo "[$(SO)] ..."
	@$(CXX) $^ -fPIC -shared -o $@ $(CXXFLAGS) $(LDFLAGS)

$(BINS): %.bin: $(OBJS) %.cpp
	@echo "[bin] $@ ..."
	@$(CXX) $^ -o $@ $(CXXFLAGS) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cc
	@echo "[cc] $< ..."
	@$(CXX) -c $< -o $@ $(CXXFLAGS) -D GIT_COMMIT_HASH=${GIT_COMMIT_HASH} -D GIT_UNSTAGED=${GIT_UNSTAGED}

$(OBJ_DIR)/%.d: %.cc Makefile
	@mkdir -p $(dir $@)
	@echo "[dep] $< ..."
	@$(CXX) $(CXXFLAGS) $(DEFINES) -MM -MT "$(OBJ_DIR)/$(<:.cc=.o) $(OBJ_DIR)/$(<:.cc=.d)" "$<"  > "$@"

clean:
	@rm -rvf $(OBJ_DIR) $(BINS) $(SO)
