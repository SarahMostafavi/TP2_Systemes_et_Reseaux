CC = gcc
SRC = main.c malloc_3is.c
BIN = tp2

CFLAGS += -Wall -Wextra -g

all: $(BIN)

$(BIN): $(SRC) malloc_3is.h
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

clean:
	rm -f $(BIN)
	rm -f *.o
	rm -f *~

indent:
	indent -linux -i4 -nut -ts2 *.c

.PHONY: all clean indent
