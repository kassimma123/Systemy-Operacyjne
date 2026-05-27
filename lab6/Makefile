CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: program1 program2 program3

program1: program1.c
	$(CC) $(CFLAGS) -o program1 program1.c

program2: program2.c
	$(CC) $(CFLAGS) -o program2 program2.c

program3: program3.c
	$(CC) $(CFLAGS) -o program3 program3.c

clean:
	rm -f program1 program2 program3
