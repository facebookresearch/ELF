TARGET= zwstest
SRCS = main.c  zwsdecoder.c  zwshandshake.c  zwssock.c
CC = gcc
DEBUG = -g
MILITANT = -Werror
CFLAGS= -std=gnu99  -Wall $(shell pkg-config --cflags libczmq zlib) $(DEBUG) $(MILITANT)
LDFLAGS = -Wall $(DEBUG)
LIBS =  $(shell pkg-config --libs libczmq zlib)

DEPDIR=./deps
OBJDIR=./objs
DEPS := $(sort $(patsubst %.c,$(DEPDIR)/%.d,$(SRCS)))
OTMP = $(patsubst %.c,%.o,$(wildcard *.c))
OBJS = $(patsubst %,$(OBJDIR)/%,$(OTMP))

all: init $(DEPS) $(OBJS)
	$(CC) -o $(TARGET) $(LDFLAGS) $(OBJS) $(LIBS)

init:
	mkdir -p $(DEPDIR)
	mkdir -p $(OBJDIR)

-include $(wildcard $(DEPS))

$(DEPDIR)/%.d: %.c
	gcc -M -MD $(CFLAGS) -o $(DEPDIR)/$*.d $<


$(OBJDIR)/%.o : %.c
	$(CC) $(DEFINES) $(CFLAGS) $(INCLUDE) -c -o $@ -c $<

clean:
	rm -rf $(DEPDIR) $(OBJDIR) $(TARGET)

etags:
	etags *.c *.h
