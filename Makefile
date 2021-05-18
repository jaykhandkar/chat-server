CC = gcc
CFLAGS = -I. -pthread
DEPS = chat-server.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: server.o utils.o tcp.o tftp.o sendrecv.o
	$(CC) -o $@ $^ $(CFLAGS)

client: client.o utils.o tcp.o tftp.o sendrecv.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o server client
