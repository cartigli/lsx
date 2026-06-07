CC     = gcc
CFLAGS = -Wall -Wextra -Wmisleading-indentation -O2 -std=c11 -g -Icolor -I.
XTFLAGS = -Werror=format-security -Werror=implicit-function-declaration 
LDLIBS = -lncurses
LDFLAGS =
TARGET = lx
OBJS   = main.o menu.o editor.o fsio.o highlight.o \
		buff.o utils.o error.o config.o  color/colors.o \
		color/c_color.o color/py_color.o color/md_color.o \
		color/blank.o

ifeq ($(shell uname -s), Linux)
	CFLAGS += -D_GNU_SOURCE
endif

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(XTFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

buff.o:      buff.h
error.o:     error.h

config.o:    config.h error.h fsio.h
editor.o:    editor.h editor.h highlight.h fsio.h \
             buff.h error.h
fsio.o:      fsio.h menu.h highlight.h utils.h error.h
highlight.o: highlight.h color/c_color.h color/py_color.h \
             color/blank.h color/colors.h
menu.o:      menu.h buff.h fsio.h highlight.h fsio.h \
             editor.h config.h error.h
main.o:      main.h highlight.h editor.h menu.h fsio.h \
             utils.h error.h config.h color/colors.h
utils.o:     utils.h error.h

color/blank.o:    color/blank.h
color/colors.o:   color/colors.h
color/c_color.o:  color/c_color.h
color/md_color.o: color/md_color.h
color/py_color.o: color/py_color.h

linux: clean
	@$(MAKE) --no-print-directory $(TARGET)

clean:
	@rm -f $(OBJS) $(TARGET)

test:
	gcc -DTEST_ALLOC -g tbuff.c buff.c error.c utils.c -o tbuff && ./tbuff

.PHONY: clean linux test
