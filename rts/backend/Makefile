
CXX ?= g++
CC ?= gcc
PYTHON ?= python
SHELL = bash
OPTFLAGS ?= -O3 -msse3

CZMQDIR = ../../vendor/CZMQ-ZWSSock
CZMQSRCS = zwsdecoder.c zwshandshake.c zwssock.c
CZMQOTMP = $(patsubst %.c,%.o, $(CZMQSRCS))
CZMQOBJS = $(patsubst %,$(CZMQDIR)/%, $(CZMQOTMP))
CFLAGS += -std=gnu99 -fPIC
PYTHON_CONFIG ?= python-config



# optional extra packages
LIBS = libzmq libczmq zlib
INCLUDE_DIR += -I. $(shell pkg-config --cflags $(LIBS)) -isystem ../../vendor -I $(GAME_DIR)
INCLUDE_DIR += $(shell $(PYTHON_CONFIG) --cflags | cut -d ' ' -f 1)

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

#LDFLAGS += -g $(shell find ../../vendor/CZMQ-ZWSSock/objs -name "*.o")

# rts_engine
ccSOURCES += $(shell find . -name "*.cc"  |  sed 's/^\.\///g')
# Assume all elf files are headers.
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

.PHONY: $(TARGET)

$(OBJ_DIR)/%.o: %.cc #$(GENERATED_FILES) #$(PROTOFILES)
	@echo "[cc] $< ..."
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

$(OBJ_DIR)/%.d: %.cc Makefile
	@mkdir -pv $(dir $@)
	@echo "[dep] $< ..."
	@$(CXX) $(CXXFLAGS) -MM -MT "$(OBJ_DIR)/$(<:.cc=.o) $(OBJ_DIR)/$(<:.cc=.d)" "$<"  > "$@"


$(CZMQDIR)/%.o: %.c
	$(CC) $(CXXFLAGS)  -c -o $@ $< -std=gnu99

$(TARGET): $(OBJS) $(GAME_OBJS) $(CZMQOBJS)
	@echo "Linking ..."
	@$(CXX) $^ -o $@ $(LDFLAGS) $(CXXFLAGS)
	@echo "done."

clean:
	@rm -rvf $(OBJ_DIR)
