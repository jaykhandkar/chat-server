#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#if BUFSIZ < (PATH_MAX + 5)
#undef BUFSIZ
#define BUFSIZ (PATH_MAX + 5)
#endif

/* this is the directory where the server will store files put up by clients */
#define SERVDIR "/home/jay/.cache/server/"
#define DEBUG 1

#define WRITE	0xa
#define PUT	0xb 
#define GET	0xc
#define LIST	0xd

#define MAGIC 0xfefefefe

#define PROMPT "🗭 "

#ifdef  DEBUG
#define ERROR(X) perror(X)
#else
#define ERROR(X)
#endif

#define PORT "9034"

void *get_ipv4_or_ipv6(struct sockaddr *);

struct rq {
	unsigned int magic;
	off_t len;
	char filename[PATH_MAX];
};
