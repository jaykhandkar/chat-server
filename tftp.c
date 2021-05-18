#include <chat-server.h>


struct tftp *tftp_init(int sockfd)
{
	struct tftp *p = malloc(sizeof(struct tftp));

	p->lastsent = 0;
	p->remotefd = sockfd;
	p->localfd = 0;
	p->nextblknum = 0;
	p->op_sent = 0;
	p->op_recv = 0;

	return p;
}

void tftp_destroy(struct tftp *p)
{
	free(p);
}

