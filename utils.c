/* utility functions needed by both client and server */

#include <chat-server.h>

void *get_ipv4_or_ipv6(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

off_t write_to_file (int sockfd, int fd, off_t len)
{
	off_t rem;
	off_t tot = 0;
	char buf[BUFSIZ];

	if (len < BUFSIZ) {
		tot = readn(sockfd, buf, len);
		write(fd, buf, len);
	}
	else {
		rem = len % BUFSIZ;
		for (int i = 0; i < (len / BUFSIZ); i++){
			tot += readn(sockfd, buf, BUFSIZ);
			write(fd, buf, BUFSIZ);
		}
		tot += readn(sockfd, buf, rem);
		write(fd, buf, rem);
	}
	return tot;
}

