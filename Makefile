CC     = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -g -Icolor -I.
XTFLAGS = -Werror=format-security -Werror=implicit-function-declaration 
LDLIBS = -lncurses
LDFLAGS =
TARGET = lx
OBJS   = main.o menu.o editor.o fsio.o highlight.o \
		buff.o utils.o error.o config.o color/c_color.o \
		color/py_color.o color/blank.o

OSX := $(shell uname -s)
ifeq ($(OSX),Linux)
# 	CFLAGS += $(LXLNKS)
	LDFLAGS += -Wl,-z,defs -Wl,z,now -Wl,-z,relro
	CFLAGS += -D_GNU_SOURCE
endif

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(XTFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

buff.o:           buff.h
color/blank.o:    color/blank.h types.h
color/c_color.o:  color/c_color.h types.h
config.o:         config.h error.h fsio.h types.h
editor.o:         editor.h editor.h highlight.h \
                  fsio.h buff.h error.h types.h
error.o:          error.h types.h
fsio.o:           fsio.h menu.h highlight.h utils.h \
                  error.h types.h
highlight.o:      highlight.h types.h color/c_color.h \
                  color/py_color.h color/blank.h
menu.o:           menu.h buff.h fsio.h highlight.h \
                  fsio.h editor.h error.h types.h
main.o:           main.h highlight.h editor.h menu.h \
                  fsio.h utils.h error.h config.h
color/py_color.o: color/py_color.h types.h
utils.o:          utils.h error.h

linux: clean
	@$(MAKE) --no-print-directory $(TARGET)

clean:
	@rm -f $(OBJS) $(TARGET)

.PHONY: clean linux
