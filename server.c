#include <chat-server.h>

void list_files(int sockfd)
{
	DIR *dirp = opendir(SERVDIR);
	struct dirent *entry;

	while ((entry = readdir(dirp)) != NULL)
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")){
				send(sockfd, entry->d_name, strlen(entry->d_name), 0);
				send(sockfd, "\n", 1, 0);
		}
	send(sockfd, PROMPT, strlen(PROMPT), 0);
	closedir(dirp);
}

void handle_put(int sockfd)
{
	struct rq rqbuf;
	struct rq zero = {0};
	int rv;
	int fd;
	int n;
	char buf[PATH_MAX];

	strcpy(buf, SERVDIR);

	n = readn(sockfd, (char *)&rqbuf, sizeof rqbuf);
	if (rqbuf.magic == MAGIC) {
		printf("received rq struct of %d bytes\n", n);
		printf("attempting to retrieve file %s\n", rqbuf.filename);
		printf("file size = %ld\n", rqbuf.len);
	}

	fd = open(strcat(buf, rqbuf.filename), O_RDWR | O_CREAT, S_IRWXU);
	printf("receiving file...\n");
	rv = write_to_file(sockfd, fd, rqbuf.len);
	if (rv == rqbuf.len && memcmp(&rqbuf, &zero, sizeof(struct rq)) != 0){
		printf("all good!\n");
	}
	else{
		printf("sorry, an error occured\n");
		unlink(rqbuf.filename);
	}
	//n = readn(sockfd, buf, rqbuf.len);
	//write(fd, buf, rqbuf.len);
	close(fd);
}

void handle_get(int sockfd, char *file)
{
	int fd, n;
	char buf[BUFSIZ];
	char pathbuf[PATH_MAX];
	struct rq rqbuf = {0};
	struct stat statbuf;
	char *x;

	file[strlen(file)-1] = 0;

	x = file;
	while (isspace(*x))
		++x;

	strcpy(pathbuf, SERVDIR);

	fd = open(strcat(pathbuf, x), O_RDONLY);
	if (fd < 0){
		send(sockfd, &rqbuf, sizeof rqbuf, 0);
		perror("open");
		return;
	}

	if (fstat(fd, &statbuf) < 0) {
		perror("fstat");
		send(sockfd, &rqbuf, sizeof rqbuf, 0);
		return;
	}
	rqbuf.magic = MAGIC;
	rqbuf.len = statbuf.st_size;
	strcpy(rqbuf.filename, x);
	send(sockfd, &rqbuf, sizeof rqbuf, 0);

	while ((n = read(fd, buf, sizeof buf)) > 0)
		send(sockfd, buf, n, 0);
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
							else if (strncmp(buf, "list", 4) == 0)
								cmd = LIST;
							else if (strncmp(buf, "get", 3) == 0)
								cmd = GET;
						}
						if (cmd == WRITE) {
							for (j = 0; j <= maxfd; j++) {
								if (FD_ISSET(j, &master))
									if (j != listener && j != i){
										if (send(j, buf + skip, n-skip, 0) < 0)
											ERROR("send");
										if (buf[n-1] == '\n')
											send(j, PROMPT, strlen(PROMPT), 0);
									}
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
						if (cmd == LIST){
							cmd = 0;
							list_files(i);
						}
						if (cmd == GET){
							cmd = 0;
							handle_get(i, buf + 3);
							memset(buf, 0, sizeof buf);
						}
					}
				}
			}
		}
	}
}
