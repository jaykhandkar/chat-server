#include <chat-server.h>

int (*fsm_table [6][6])(struct tftp *, char *, int) = {
#ifdef CLIENT
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,

	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	recv_data,
	fsm_invalid,
	recv_rqerr,

	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	recv_ack,
	recv_rqerr,

	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	recv_ack,
	fsm_error,

	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	recv_data,
	fsm_invalid,
	fsm_error,

	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
};
#endif
#ifdef SERVER
	fsm_invalid,
	recv_read_rq,
	recv_write_rq,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,

	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,

	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	recv_ack,
	fsm_error,

	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	recv_data,
	fsm_invalid,
	fsm_error,

	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
	fsm_invalid,
};
#endif

int fsm_invalid(struct tftp *p, char *buf, int nbytes)
{
	return -1;
}

int fsm_error(struct tftp *p, char *buf, int nbytes)
{
	return -1;
}

void fsm_loop(struct tftp *p)
{
	char recvbuf[MAXBUFF];
	int nbytes;
	int recvopcode;

	while (1) {
		nbytes = tcp_recv(p->remotefd, recvbuf, MAXBUFF);
		
		if (nbytes == 0) {
			printf("server died\n");
			exit(1);
		}

		if (nbytes < 4) {
			printf("received packet of size = %d bytes\n", nbytes);
			return;
		}

		recvopcode = get_short(recvbuf);
		if (recvopcode > 6 || recvopcode < 1) {
			printf("invalid opcode received\n");
			return;
		}
		p->op_recv = recvopcode;

		if ((*fsm_table [p->op_sent][p->op_recv])(p, recvbuf + 2, nbytes - 2) < 0)
			return;
	}
}
