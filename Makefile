CC = gcc
# no-strict-aliasing flag is required, type punning is used
# when reading opcodes and block numbers from buffers
CFLAGS = -I. -pthread -fno-strict-aliasing
DEPS = chat-server.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: server.o utils.o tcp.o tftp.o sendrecv.o
	$(CC) -c -o fsm.o fsm.c $(CFLAGS) -DSERVER
	$(CC) -o $@ $^ fsm.o $(CFLAGS) 

client: client.o utils.o tcp.o tftp.o sendrecv.o
	$(CC) -c -o fsm.o fsm.c $(CFLAGS) -DCLIENT
	$(CC) -o $@ $^ fsm.o $(CFLAGS)

clean:
	rm -f *.o server client
