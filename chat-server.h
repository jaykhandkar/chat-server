#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DEBUG 1

#ifdef  DEBUG
#define ERROR(X) perror(X);
#else
#define ERROR(X)
#endif

#define PORT "9034"
