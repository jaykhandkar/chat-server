CC = gcc
CFLAGS = -I.
DEPS = chat-server.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: server.o utils.o
	$(CC) -o $@ $^ $(CCFLAGS)

client: client.o utils.o
	$(CC) -o $@ $^ $(CCFLAGS)

clean:
	rm -f *.o server client
