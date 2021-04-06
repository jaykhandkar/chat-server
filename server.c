#include "chat-server.h"

void *get_ipv4_or_ipv6(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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

void handle_put(int sockfd)
{
	struct rq rqbuf;
	int fd;
	int n;

	n = readn(sockfd, (char *)&rqbuf, sizeof rqbuf);
	if (rqbuf.magic == MAGIC) {
		printf("read %d bytes\n", n);
		printf("%s\n", rqbuf.filename);
		printf("%d\n", rqbuf.len);
	}
	//fd = open(rqbuf.filename, O_RDWR | O_CREAT, S_IRWXU);
}

int main()
{
	int skip;
	int maxfd;
	fd_set master, readfds;
	int cmd = 0; /* command being processed */

	int listener;
	int fd;
	struct sockaddr_storage cliaddr;
	struct addrinfo hints, *ai, *p;
	socklen_t len;

	char buf[BUFSIZ];
	char remote[INET6_ADDRSTRLEN];
	int n;
	int yes = 1, i, j, rv;

	FD_ZERO(&master);
	FD_ZERO(&readfds);
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		printf("server: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for (p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0)
			continue;

		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	if (!p) {
		printf("server: failed to bind\n");
		exit(1);
	}

	freeaddrinfo(ai);

	if (listen(listener, 20) < 0) {
		ERROR("listen");
		exit(1);
	}

	FD_SET(listener, &master);
	maxfd = listener;

	while (1) {
		readfds = master;
		if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0){
			perror("select");
			exit(1);
		}

		for (i = 0; i <= maxfd; i++) {
			if (FD_ISSET(i, &readfds)){
				if (i == listener) {
					len = sizeof cliaddr;
					fd = accept(listener, (struct sockaddr*)&cliaddr, &len);
					if (fd < 0){
						ERROR("accept");
					}
					else {
						FD_SET(fd, &master);
						if (fd > maxfd)
							maxfd = fd;
						printf("server: new connection from %s\n", inet_ntop(cliaddr.ss_family,
									get_ipv4_or_ipv6((struct sockaddr *)&cliaddr),
									remote, INET6_ADDRSTRLEN));

					}
				}
				else {
					if ((n = recv(i, buf, sizeof buf, 0)) <= 0) {
						if (n == 0)
							printf("socket %d disconnected\n", i);
						else
							ERROR("recv");
						close(i);
						FD_CLR(i, &master);
					}
					else {
						if (!cmd) {
							if (strncmp(buf, "write", 5) == 0){
								cmd = WRITE;
								skip = 5;
							}
							else if (strncmp(buf, "put", 3) == 0)
								cmd = PUT;
						}
						if (cmd == WRITE) {
							for (j = 0; j <= maxfd; j++) {
								if (FD_ISSET(j, &master))
									if (j != listener && j != i)
										if (send(j, buf + skip, n-skip, 0) < 0)
											ERROR("send");
							}
							skip = 0;
							if (buf[n-1] == '\n'){
								memset(buf, 0, sizeof buf);
								cmd = 0;
							}
						}
						if (cmd == PUT) {
							cmd = 0;
							handle_put(i);
							memset(buf, 0, sizeof buf);
						}
					}
				}
			}
		}
	}
}
