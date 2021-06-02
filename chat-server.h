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

#define UNAME_MAX 256      /* max length of an username*/

/* op codes for tftp commands */

#define  OP_RRQ		1  /* read request */
#define  OP_WRQ		2  /* write request */
#define  OP_DATA	3  /* data packet */
#define  OP_ACK		4  /* acknowledgement packet */
#define  OP_ERROR	5  /* error packet */

/* tftp error codes */

#define  ERR_NOTDEF	0  /* not defined */
#define  ERR_FNF	1  /* file not found */
#define  ERR_ACC	2  /* access violation */
#define  ERR_FULL	3  /* disk full/allocation exceeded */
#define  ERR_BADOP	4  /* illegal tftp operation */
#define  ERR_BADTRANS	5  /* unknown transfer id (port) (unused)*/
#define  ERR_BADUSR	6  /* no such user (unused) */


#define MAXBUFF 2048       /* size of send and receive buffers */
#define MAXDATA 512        /* maximum amount of data in a data packet */

/* this is the directory where the server will store files put up by clients */
#define SERVDIR "/home/jay/.cache/server/"

#define PROMPT "💭 "

#define PORT "9071"	/* outside reserved range */

/* encapsulates the state of a tftp transaction */

struct tftp {
	int lastsent;			/* amount of data in last data packet */
	int remotefd;			/* socket descriptor of remote */
	int localfd;			/* file that is being read from */
	char localfname[PATH_MAX];	/* and the corresponding filename */
	int nextblknum;			/* expected block number of next data packet */
	int op_sent;			/* op code of last packet sent */
	int op_recv;			/* op code of last packet received */
	int totbytes;			/* total # of bytes sent/received so far */
};

/* tcp.c */

void tcp_send(int, char*, int);
int tcp_recv(int, char *, int);
int readn(int, char *, int);
int writen(int, char *, int);

/* sendrecv.c */

void send_rq(struct tftp*, short, char *);
int recv_rqerr(struct tftp *, char *, int);
void send_ack(struct tftp *, short);
int send_data(struct tftp *, short);
int recv_data(struct tftp *, char *,  int);
int recv_ack(struct tftp *, char *, int);
void send_err(struct tftp *, short, char *);
int recv_read_rq(struct tftp *, char *, int);
int recv_write_rq(struct tftp *, char *, int);

/* tftp.c */
struct tftp *tftp_init(int);
void tftp_destroy(struct tftp *);

/* fsm.c */
int fsm_error(struct tftp *, char *, int);
int fsm_invalid(struct tftp *, char *, int);
void fsm_loop(struct tftp *);

void *get_ipv4_or_ipv6(struct sockaddr *);

/* structure to keep track of connected clients */

struct cli_fds {
	pthread_mutex_t	f_lock;
	fd_set		fds;
	int		maxfd;
};

