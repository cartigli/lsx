CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LNKLIB = -lncurses
TARGET = lx
OBJS   = main.o menu.o editor.o fsio.o highlight.o buff.o utils.o error.o config.o types.o

OSX := $(shell uname -s)
ifeq ($(OSX),Linux)
	CFLAGS += -D_GNU_SOURCE
endif

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LNKLIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

buff.o:      buff.h
config.o:    config.h error.h fsio.h types.h
editor.o:    editor.h editor.h highlight.h fsio.h buff.h error.h types.h
error.o:     error.h types.h
fsio.o:      fsio.h menu.h highlight.h utils.h error.h types.h
highlight.o: highlight.h
menu.o:      menu.h buff.h fsio.h highlight.h fsio.h editor.h error.h types.h
main.o:      main.h highlight.h editor.h menu.h fsio.h utils.h error.h config.h
type.o:      types.h
utils.o:     utils.h error.h

linux: clean
	@$(MAKE) --no-print-directory $(TARGET)

clean:
	@rm -f $(OBJS) $(TARGET)

.PHONY: clean linux
