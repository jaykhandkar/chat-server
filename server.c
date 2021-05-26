#include <chat-server.h>

struct cli_fds clients;
extern int (*fsm_table [6][6])(struct tftp *, char *, int);

void list_files(int sockfd)
{
	DIR *dirp = opendir(SERVDIR);
	struct dirent *entry;

	while ((entry = readdir(dirp)) != NULL) {
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")){
				send(sockfd, entry->d_name, strlen(entry->d_name), 0);
				send(sockfd, "\n", 1, 0);
		}
	}

	send(sockfd, PROMPT, strlen(PROMPT), 0);
	closedir(dirp);
}

/*void handle_put(int sockfd)
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

	if (!S_ISREG(statbuf.st_mode)) {
		send(sockfd, &rqbuf, sizeof rqbuf, 0);
		return;
	}

	rqbuf.magic = MAGIC;
	rqbuf.len = statbuf.st_size;
	strcpy(rqbuf.filename, x);
	send(sockfd, &rqbuf, sizeof rqbuf, 0);

	while ((n = read(fd, buf, sizeof buf)) > 0)
		send(sockfd, buf, n, 0);
}*/

void *thread_handler(void *arg)
{
	int fd = (int) arg;
	int n, nuser;
	int skip;
	int cmd = 0;
	int msglen, msgblknum;
	short recvopcode;
	char buf[MAXBUFF];
	char username[UNAME_MAX]; //username, needs to be thread local
	struct tftp *p;

	pthread_mutex_lock(&clients.f_lock);
	FD_SET(fd, &clients.fds);
	pthread_mutex_unlock(&clients.f_lock);

	nuser = read(fd, username, UNAME_MAX);
	printf("username for socket %d is %s\n", fd, username);

	p = tftp_init(fd);
	for ( ; ; ) {
		n = tcp_recv(fd, buf, MAXBUFF);
		
		if (n == 0) {
			printf("socket %d :%s hung up\n", fd, username);
			pthread_mutex_lock(&clients.f_lock);
			FD_CLR(fd, &clients.fds);
			pthread_mutex_unlock(&clients.f_lock);
			tftp_destroy(p);
			return (void *)0;
		}

		if (strncmp(buf, "write", 5) == 0) {
			msgblknum = get_int(buf + 5);
			msglen = n - 5 - sizeof(int);
			for (int i = 0; i <= clients.maxfd; i++){
				if (i != fd && FD_ISSET(i, &clients.fds)) {
					if (msgblknum == 1) {
						write(i, username, nuser);
						write(i, ":", 1);
					}
					write(i, buf + 5 + sizeof(int), msglen);
					if (msglen < MAXDATA)
						write(i, PROMPT, strlen(PROMPT));
				}
			}
			continue;
		}

		if (strncmp(buf, "list", 4) == 0) {
			list_files(fd);
			continue;
		}
		
		recvopcode = get_short(buf);
		if (recvopcode > 5 || recvopcode < 1) {
			printf("received invalid op code\n");
			return (void *) 0;
		}

		p->op_recv = recvopcode;
		printf("op_recv = %d, op_sent = %d\n", p->op_recv, p->op_sent);

		if ((*fsm_table[p->op_sent][p->op_recv])(p, buf + 2, n - 2) < 0) {
			p->op_sent = 0;
			p->op_recv = 0;
			p->localfd = 0;
			p->nextblknum = 0;
			p->lastsent = 0;
			continue;
		}

		
		/*if (cmd == 0) {
			if (strncmp(buf, "write", 5) == 0){
				cmd = WRITE;
				skip = 5;
			}
			else if (strncmp(buf, "list", 4) == 0)
				cmd = LIST;
			else if (strncmp(buf, "put", 3) == 0)
				cmd = PUT;
			else if (strncmp(buf, "get", 3) == 0)
				cmd = GET;
		}
		if (cmd == WRITE) {
			for (int i = 0; i <= clients.maxfd; i++){
				if (i != fd && FD_ISSET(i, &clients.fds)) {
					if (skip != 0) {
						write(i, username, nuser);
						write(i, ":", 1);
					}
					write(i, buf + skip, n - skip);
					if (buf[n-1] == '\n')
						write(i, PROMPT, strlen(PROMPT));
				}
			}
			skip = 0;
			if (buf[n-1] == '\n')
				cmd = 0;
		}
		if (cmd == LIST){
			cmd = 0;
			list_files(fd);
		}
		if (cmd == GET){
			cmd = 0;
			handle_get(fd, buf + 3);
			memset(buf, 0, sizeof buf);
		}
		if (cmd == PUT) {
			cmd = 0;
			handle_put(fd);
			memset(buf, 0, sizeof buf);
		}*/

	}
	tftp_destroy(p);
	return (void *)0;
}

int main()
{
	pthread_t tid;

	int listener;
	int fd;
	struct sockaddr_storage cliaddr;
	struct addrinfo hints, *ai, *p;
	socklen_t len = sizeof(struct sockaddr_storage);

	char remote[INET6_ADDRSTRLEN];
	int yes = 1, rv;

	if (pthread_mutex_init(&clients.f_lock, NULL) != 0) {
		printf("failed to initialise mutex\n");
		exit(1);
	}

	pthread_mutex_lock(&clients.f_lock);
	clients.maxfd = 0;
	pthread_mutex_unlock(&clients.f_lock);

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
		perror("listen");
		exit(1);
	}

	while (1) {
		fd = accept(listener, (struct sockaddr*) &cliaddr, &len);
		printf("server: new connection from %s\n", inet_ntop(cliaddr.ss_family,
					get_ipv4_or_ipv6((struct sockaddr *)&cliaddr),
					remote, INET6_ADDRSTRLEN));
		
		pthread_mutex_lock(&clients.f_lock);
		if (fd > clients.maxfd)
			clients.maxfd = fd;
		pthread_mutex_unlock(&clients.f_lock);

		if (pthread_create(&tid, NULL, thread_handler, (void *) fd) != 0) {
			printf("failed to spawn new thread for %s\n", remote);
			continue;
		}
		

	}
}
