OUTPUT=network
CC = gcc
CFLAGS = -g -std=c99 -Wvla -Wall -fsanitize=address,undefined
LFLAGS= -lm -pthread

objects = network ht
all: $(objects)

$(objects): %: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS)

clean:
	rm -f *.o $(OUTPUT)
