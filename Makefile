CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
linux: CFLAGS += -D_GNU_SOURCE
LNKLIB = -lncurses
TARGET = lx_menu
OBJS   = lx_menu.o file_edit.o iofs.o highlight.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LNKLIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS):     iofs.h highlight.h
file_edit.o: iobuff.h highlight.h
highlight.o: highlight.h

linux: $(TARGET)

clean:
	rm -rf $(OBJS) $(TARGET)

.PHONY: clean
