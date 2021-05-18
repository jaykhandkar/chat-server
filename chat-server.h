#include <stdio.h>
#include <pthread.h>
#include <signal.h>
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

#define get_int(addr)		( ntohl(* (int *) (addr)) )
#define set_int(val, addr)	( *( (int *) (addr) ) = htonl(val) )

#define get_short(addr)		( ntohs(* (short *) (addr)) )
#define set_short(addr, val)	( *( (short *) (addr) ) = htons(val) )

#define UNAME_MAX 256 /* max length of an username*/
#define MAX_USERS 20  /* max users connected to the server at a time */

/* op codes for tftp commands */

#define  OP_RRQ		1  /* read request */
#define  OP_WRQ		2  /* write request */
#define  OP_DATA	3  /* data packet */
#define  OP_ACK		4  /* acknowledgement packet */
#define  OP_ERROR	5  /* error packet */

/* tftp error codes */

#define  ERR_NOTDEF	0 /* not defined */
#define  ERR_FNF	1 /* file not found */
#define  ERR_ACC	2 /* access violation */
#define  ERR_FULL	3 /* disk full/allocation exceeded */
#define  ERR_BADOP	4 /* illegal tftp operation */
#define  ERR_BADTRANS	5 /* unknown transfer id (port) (unused)*/
#define  ERR_BADUSR	6 /* no such user (unused) */


#define MAXBUFF 2048  /* size of send and receive buffers */
#define MAXDATA 512   /* maximum amount of data in a data packet */

#if BUFSIZ < (PATH_MAX + 5)
#undef BUFSIZ
#define BUFSIZ (PATH_MAX + 5)
#endif

#undef BUFSIZ
#define BUFSIZ 10

/* this is the directory where the server will store files put up by clients */
#define SERVDIR "/home/jay/.cache/server/"
#define DEBUG 1

#define WRITE	0xa
#define PUT	0xb 
#define GET	0xc
#define LIST	0xd

#define MAGIC 0xfefefefe

#define PROMPT "💭 "

#ifdef  DEBUG
#define ERROR(X) perror(X)
#else
#define ERROR(X)
#endif

#define PORT "9034"

/* tcp.c */
void tcp_send(int, char*, int);
int tcp_recv(int, char *, int);

void *get_ipv4_or_ipv6(struct sockaddr *);
int write_to_file(int, int, int);
int readn(int, char *, int);

/* structure to keep track of connected clients */

struct cli_fds {
	pthread_mutex_t	f_lock;
	fd_set		fds;
	int		maxfd;
};

/* encapsulates the state of a tftp transaction */

struct tftp {
	int lastsent;	/* amount of data in last data packet */
	int remotefd;	/* socket descriptor of remote */
	int localfd;	/* file that is being read from */
	int nextblknum;	/* expected block number of next data packet */
	int op_sent;	/* op code of last packet sent */
	int op_recv;	/* op code of last packet received */
};

struct rq {
	unsigned int magic;
	off_t len;
	char filename[1024];
};
