CC      ?= gcc
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -Werror -O0

all: test_packet

test_packet: packet.c test_packet.c packet.h
	$(CC) $(CFLAGS) -o $@ packet.c test_packet.c

test: test_packet
	./test_packet

coverage: clean
	$(CC) $(CFLAGS) --coverage -c packet.c -o packet.o
	$(CC) $(CFLAGS) -c test_packet.c -o test_packet.o
	$(CC) --coverage -o test_packet packet.o test_packet.o
	./test_packet
	gcov -b packet.c

clean:
	rm -f test_packet *.o *.gcda *.gcno *.gcov
