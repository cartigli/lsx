CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
linux: CFLAGS += -D_GNU_SOURCE
LNKLIB = -lncurses
TARGET = lx
OBJS   = menu.o editor.o fsio.o highlight.o buff.o init.o utils.o

OSX := $(shell uname -s)
ifeq ($(OSX),Linux)
	CFLAGS += -D_GNU_SOURCE
endif

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LNKLIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

buff.o:      buff.h
editor.o:    editor.h init.h editor.h highlight.h fsio.h buff.h
fsio.o:      fsio.h menu.h highlight.h utils.h
highlight.o: highlight.h
init.o:      init.h highlight.h buff.h editor.h menu.h fsio.h
menu.o:      menu.h buff.h init.h fsio.h highlight.h fsio.h
utils.o:     utils.h

linux: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
