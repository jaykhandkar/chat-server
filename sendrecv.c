#include <chat-server.h>

/* sendrecv.c - routines that deal with processing and sending packets */

void send_rq(struct tftp *p, short opcode, char *file)
{
	int nbytes;
	char sendbuf[MAXBUFF];

	set_short(sendbuf, opcode);
	strcpy(sendbuf + 2, file);
	nbytes = 2 + strlen(file) + 1;

	tcp_send(p->remotefd, sendbuf, nbytes);
	p->op_sent = opcode;
}

int recv_rqerr(struct tftp *p, char *buf, int nbytes)
{
	int errcode;

	errcode = get_short(buf);
	buf += 2;
	nbytes -= 2;
	buf[nbytes] = 0;

	unlink(p->localfname);
	printf("error code %d: %s\n", errcode, buf);
	return -1;
}

void send_ack(struct tftp *p, short blknum)
{
	char sendbuf[MAXBUFF];
	int nbytes = 4;

	set_short(sendbuf, OP_ACK);
	set_short(sendbuf + 2, blknum);

	tcp_send(p->remotefd, sendbuf, nbytes);
	p->op_sent = OP_ACK;
}

int send_data(struct tftp *p, short blknum)
{
	char sendbuf[MAXBUFF];
	int nread;

	printf("sending block #%d\n", blknum);
	set_short(sendbuf, OP_DATA);
	set_short(sendbuf + 2, blknum);

	nread = read(p->localfd, sendbuf + 4, MAXDATA);
	if (nread == 0 && p->lastsent < MAXDATA)
		return nread;
	tcp_send(p->remotefd, sendbuf, nread + 4);
	p->op_sent = OP_DATA;
	p->lastsent = nread;

	return nread;
}

int recv_data(struct tftp *p, char *buf, int nbytes)
{
	short recvblknum;

	recvblknum = get_short(buf);
	buf += 2;
	nbytes -= 2;

	if (nbytes > MAXDATA) {
		printf("received packet with %d bytes of data\n", nbytes);
		exit(1);
	}

	if (recvblknum == p->nextblknum) {
		printf("okay, recvblknum = %d nexblknum = %d\n", recvblknum, p->nextblknum);
		p->nextblknum++;

		if (nbytes > 0)
			write(p->localfd, buf, nbytes);
		if (nbytes < MAXDATA)
			close(p->localfd);
	} else {
		printf("received unexpected data block\n");
		exit(1);
	}

	send_ack(p, recvblknum);
	
	if (nbytes == MAXDATA)
		return 0;
	else
		return -1;
}

int recv_ack(struct tftp *p, char *buf, int nbytes)
{
	short recvblknum;
	int n;

	if (nbytes != 2) {
		printf("ACK packet received of size %d\n", nbytes);
		exit(1);
	}

	recvblknum = get_short(buf);
	if (recvblknum == p->nextblknum) {
		printf("okay, corect ACK received\n");
		p->nextblknum++;
		n = send_data(p, p->nextblknum);

		if (n == 0) {
			if (p->lastsent < MAXDATA)
				return -1;
		}
		return 0;
	}
	else if (recvblknum == p->nextblknum - 1) {
		return 0;
	}
	else {
		printf("got unexpected ack: %d\n", recvblknum);
		exit(1);
	}
}

void send_err(struct tftp *p, short errcode, char *str)
{
	char sendbuf[MAXBUFF];
	int nbytes;

	set_short(sendbuf, OP_ERROR);
	set_short(sendbuf + 2, errcode);

	strcpy(sendbuf + 4, str);
	nbytes = 4 + strlen(str) + 1;
	tcp_send(p->remotefd, sendbuf, nbytes);

}

int recv_read_rq(struct tftp *p, char *buf, int nbytes)
{
	char filename[PATH_MAX];
	struct stat stbuf;
	int fd;
	int fileok = 0;

	for (int i = 0; i < nbytes; i++)
		if (buf[i] == '\0')
			fileok = 1;

	if (fileok != 1) {
		send_err(p, ERR_BADOP, "bad filename: not null terminated");
		return -1;
	}

	if (strlen(buf) + strlen(SERVDIR) + 1 > PATH_MAX) {
		send_err(p, ERR_NOTDEF, "path to file too long");
		return -1;
	}

	strcpy(filename, SERVDIR);
	strcat(filename, buf);

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		send_err(p, ERR_FNF, strerror(errno));
		return -1;
	}

	p->localfd = fd;
	p->nextblknum++;  /* block #1 */
	send_data(p, p->nextblknum);

	return 0;
}

int recv_write_rq(struct tftp *p, char *buf, int nbytes)
{
	char filename[PATH_MAX];
	int fd;
	int fileok = 0;

	for (int i = 0; i < nbytes; i++)
		if (buf[i] == '\0')
			fileok = 1;

	if (fileok != 1) {
		send_err(p, ERR_BADOP, "bad filename: not null terminated");
		return -1;
	}

	if (strlen(buf) + strlen(SERVDIR) + 1 > PATH_MAX) {
		send_err(p, ERR_NOTDEF, "path to file too long");
		return -1;
	}
	printf("received WRQ for %s\n", buf);
	
	strcpy(filename, SERVDIR);
	strcat(filename, buf);
	strcpy(p->localfname, filename);

	fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		send_err(p, ERR_NOTDEF, strerror(errno));
		return -1;
	}

	p->localfd = fd;
	p->nextblknum++;

	send_ack(p, 0);

	return 0;
}

