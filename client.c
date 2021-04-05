#include "chat-server.h"

void recv_loop(int fd)
{
	char buf[BUFSIZ];
	int n;

	while ((n = recv(fd, buf, BUFSIZ, 0)) > 0){
		write(1, buf, n);
		write(1, ">", 1);
	}
}

void *get_ipv4_or_ipv6(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	char buf[BUFSIZ];
	int sockfd, nbytes;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
		printf("usage: client <hostname>\n");
		exit(1);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		printf("%s\n", gai_strerror(rv));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			ERROR("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
			ERROR("client: connect");
			continue;
		}
		break;
	}

	if (!p) {
		printf("client: failed to connect\n");
		return 1;
	}

	inet_ntop(p->ai_family, get_ipv4_or_ipv6((struct sockaddr *)p->ai_addr), s, sizeof(s));
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo);

	if (!fork())
		recv_loop(sockfd);

	while (1) {
		printf(">");
		while (fgets(buf, BUFSIZ, stdin)) {
			send(sockfd, buf, strlen(buf), 0);
			if (buf[strlen(buf) - 1] == '\n')
				printf(">");
		}
	}
}
