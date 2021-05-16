CC = gcc
CFLAGS = -I. -Wall -Wextra -pthread
DEPS = chat-server.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: server.o utils.o tcp.o
	$(CC) -o $@ $^ $(CFLAGS)

client: client.o utils.o tcp.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o server client
