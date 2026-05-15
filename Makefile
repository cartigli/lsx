CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
linux: CFLAGS += -D_GNU_SOURCE
LNKLIB = -lncurses
TARGET = lx_menu
OBJS   = lx_menu.o file_edit.o iofs.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LNKLIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS):     iofs.h
file_edit.o: iobuff.h

linux: $(TARGET)

clean:
	rm -rf $(OBJS) $(TARGET)

.PHONY: clean
