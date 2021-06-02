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

		if ((*fsm_table[p->op_sent][p->op_recv])(p, buf + 2, n - 2) < 0) {
			p->op_sent = 0;
			p->op_recv = 0;
			p->localfd = 0;
			p->nextblknum = 0;
			p->lastsent = 0;
			continue;
		}

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
