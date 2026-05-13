CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LNKLIB = -lncurses
TARGET = file_edit
OBJS = file_edit.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LNKLIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS): fstypes.h

clean:
	rm -rf $(OBJS) $(TARGET)

.PHONY: clean

