#include <chat-server.h>

/* tcp.c - routines that deal with sending/receiving records over
 * TCP stream sockets */

int readn(int fd, char *ptr, int nbytes)
{
	int nread, nleft;

	nleft = nbytes;
	while (nleft > 0) {
		nread = recv(fd, ptr, nleft, 0);
		if (nread < 0)
			return nread;
		else if (nread == 0)
			break;
		nleft -= nread;
		ptr += nread;
	}
	return (nbytes - nleft);
}

int writen(int fd, char *ptr, int nbytes)
{
	int nwritten, nleft;

	nleft = nbytes;
	while (nleft > 0) {
		nwritten = send(fd, ptr, nleft, 0);
		if (nwritten < 0)
			return nwritten;
		else if (nwritten == 0)
			break;
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (nbytes - nleft);
}
