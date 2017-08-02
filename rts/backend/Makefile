
CXX ?= g++
CC ?= gcc
PYTHON ?= python
SHELL = bash
OPTFLAGS ?= -O3 -msse3

CFLAGS += -fPIC
PYTHON_CONFIG ?= python-config

# optional extra packages
LIBS = zlib
INCLUDE_DIR += -I. $(shell pkg-config --cflags $(LIBS)) -isystem ../../vendor -I $(GAME_DIR)
INCLUDE_DIR += $(shell $(PYTHON_CONFIG) --cflags | cut -d ' ' -f 1)

#DEFINES += -DDEBUG
CXXFLAGS ?=
CXXFLAGS += $(INCLUDE_DIR)
CXXFLAGS += -Wall -Wextra
#CXXFLAGS += -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare -Wno-unused-function -Wno-misleading-indentation -Wno-unused-but-set-variable
CXXFLAGS += -D GIT_COMMIT_HASH=${GIT_COMMIT_HASH} -D GIT_UNSTAGED=${GIT_UNSTAGED}
CXXFLAGS += $(DEFINES) -std=c++11 $(OPTFLAGS) -fPIC

LDFLAGS += $(shell pkg-config $(LIBS) --libs)
LDFLAGS += $(shell $(PYTHON_CONFIG) --ldflags)
LDFLAGS += $(OPTFLAGS)
LDFLAGS += -lm -ltbb

# rts_engine
ccSOURCES += $(shell find . -name "*.cc"  |  sed 's/^\.\///g')
# elf
ccSOURCES += $(shell find ../../elf/lib -name "*.cc"  |  sed 's/^\.\///g')

OBJ_DIR = obj
OBJS = $(addprefix $(OBJ_DIR)/,$(ccSOURCES:.cc=.o))
GAME_OBJ_DIR = $(GAME_DIR)/obj
ENGINE_OBJ_DIR = $(GAME_DIR)/engine
GAME_OBJS = $(wildcard $(GAME_OBJ_DIR)/*.o $(ENGINE_OBJ_DIR)/*.o)
DEPFILES = $(OBJS:.o=.d)
GAME_DEPFILES = $(GAME_OBJS:.o=.d)

ifneq ($(MAKECMDGOALS), clean)
sinclude $(DEPFILES)
sinclude $(GAME_DEPFILES)
endif

TARGET = minirts
all: $(TARGET)

.PHONY: $(TARGET)

$(OBJ_DIR)/%.o: %.cc #$(GENERATED_FILES) #$(PROTOFILES)
	@echo "[cc] $< ..."
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

$(OBJ_DIR)/%.d: %.cc Makefile
	@mkdir -pv $(dir $@)
	@echo "[dep] $< ..."
	@$(CXX) $(CXXFLAGS) -MM -MT "$(OBJ_DIR)/$(<:.cc=.o) $(OBJ_DIR)/$(<:.cc=.d)" "$<"  > "$@"


$(TARGET): $(OBJS) $(GAME_OBJS)
	@echo "Linking ..."
	@$(CXX) $^ -o $@ $(LDFLAGS) $(CXXFLAGS)
	@echo "done."

clean:
	@rm -rvf $(OBJ_DIR)
