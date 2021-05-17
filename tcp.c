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

void tcp_send(int sockfd, char *buf, int nbytes)
{
	int n;
	int templen;

	templen = htonl(nbytes);
	n = writen(sockfd, (char *)&templen, sizeof(int));
	if (n != sizeof(int)) {
		printf("writen prefix error\n");
		exit(1);
	}

	n = writen(sockfd, buf, nbytes);
	if (n != nbytes) {
		printf("writen data error\n");
		exit(1);
	}
}

int tcp_recv(int sockfd, char *buf, int maxlen)
{
	int templen;
	int n;

	n = readn(sockfd, (char *) &templen, sizeof(int));
	if (n == 0)
		return n;

	if (n != sizeof(int)) {
		printf("readn length prefix error\n");
		exit(1);
	}

	templen = ntohl(templen);
	if (templen > maxlen) {
		printf("record too large\n");
		exit(1);
	}

	n = readn(sockfd, buf, templen);
	if (n != templen) {
		printf("readn data error\n");
		exit(1);
	}
	return n;
}
