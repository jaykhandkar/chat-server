#include "chat-server.h"

void recv_loop(int fd)
{
	char buf[BUFSIZ];
	int n;

	while ((n = recv(fd, buf, BUFSIZ, 0)) > 0){
		write(1, buf, n);
		write(1, PROMPT, strlen(PROMPT));
	}
}

void send_file(int sockfd, char *path)
{
	int fd;
	char buf[BUFSIZ];
	struct stat statbuf = {0};
	struct rq rqbuf = {0};
	int n;

	while (isspace(*path))
		path++;

	path[strlen(path)-1] = 0;
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		send(sockfd, &rqbuf, sizeof rqbuf, 0);
		return;
	}

	if (fstat(fd, &statbuf) < 0) {
		perror("fstat");
		send(sockfd, &rqbuf, sizeof rqbuf, 0);
		return;
	}

	if (!S_ISREG(statbuf.st_mode)) {
		printf("please enter a regular file\n");
		send(sockfd, &rqbuf, sizeof rqbuf, 0);
		return;
	}

	rqbuf.magic = MAGIC;
	rqbuf.len = statbuf.st_size;
	strcpy(rqbuf.filename, basename(path));
	send(sockfd, &rqbuf, sizeof rqbuf, 0);

	while ((n = read(fd, buf, sizeof buf)) > 0)
		send(sockfd, buf, n, 0);

	close(fd);
}

void usage()
{
	printf("valid commands: write <text> - writes text to all other clients\n"
	       "                put   <file> - puts file on the server\n"
	       "	        get   <file> - retrieves file from the server\n"
	       "	        list         - displays all files on the server\n");
}

void *get_ipv4_or_ipv6(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	char *line = NULL;
	int sockfd, nbytes;
	struct addrinfo hints, *servinfo, *p;
	char *x;
	int rv;
	char s[INET6_ADDRSTRLEN];
	size_t len = 0;
	ssize_t read;

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

	printf(PROMPT);
	while ((read = getline(&line, &len, stdin)) > 0){
		x = line;
		while (isspace(*x ))
			x++;
		if (strncmp(x, "write", 5) == 0){
			send(sockfd, x, strlen(x), 0);
		}
		else if (strncmp(x, "put", 3) == 0) {
			send(sockfd, x, strlen(x), 0);
			send_file(sockfd, x + 3);
		}
		else
			usage();
		//send(sockfd, line, strlen(line), 0);
		printf(PROMPT);
	}
	free(line);
}
