#include <chat-server.h>

void recv_loop(int fd)
{
	char buf[BUFSIZ];
	int n;

	while ((n = recv(fd, buf, BUFSIZ, 0)) > 0)
		write(1, buf, n);
}

void do_put(char *file, int servfd)
{
	struct tftp *p;
	struct stat stbuf;

	p = tftp_init(servfd);
	if (strlen(file) + 1 > PATH_MAX) {
		printf("path too long\n");
		return;
	}
	strcpy(p->localfname, file);

	p->localfd = open(file, O_RDONLY);
	if (p->localfd < 0) {
		printf("failed to open local file for reading: %s\n", file);
		tftp_destroy(p);
		return;
	}

	if (fstat(p->localfd, &stbuf) < 0) {
		perror("couldnt fstat");
		tftp_destroy(p);
		return;
	}

	if (!S_ISREG(stbuf.st_mode)) {
		printf("cannot send non-regular file\n");
		tftp_destroy(p);
		return;
	}

	send_rq(p, OP_WRQ, basename(file));
	printf("sending file...\n");
	fsm_loop(p);
	close(p->localfd);
	tftp_destroy(p);
}

void do_get(char *file, int servfd)
{
	struct tftp *p;

	p = tftp_init(servfd);
	if (strlen(file) + 1 > PATH_MAX) {
		printf("path too long\n");
		return;
	}
	strcpy(p->localfname, file);

	p->localfd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (p->localfd < 0) {
		printf("failed to open local file for writing: %s\n", file);
		tftp_destroy(p);
		return;
	}

	send_rq(p, OP_RRQ, file);
	p->nextblknum++;
	fsm_loop(p);
	close(p->localfd);
	tftp_destroy(p);
}

void usage()
{
	printf("valid commands: write <text> - writes text to all other clients\n"
	       "                put   <file> - puts file on the server\n"
	       "	        get   <file> - retrieves file from the server\n"
	       "	        list         - displays all files on the server\n");
}

int main(int argc, char *argv[])
{
	char *line = NULL;
	int pid;
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	char *x;
	int rv;
	char s[INET6_ADDRSTRLEN];
	size_t len = 0;
	ssize_t read;
	char username[UNAME_MAX];
	char sendbuff[MAXBUFF];
	int msglen;
	int i;

	if (argc != 3) {
		printf("usage: client <hostname> <username>\n");
		exit(1);
	}

	if (strlen(argv[2]) + 1> UNAME_MAX) {
		printf("username should be less than %d characters long\n", UNAME_MAX);
		exit(1);
	}
	strcpy(username, argv[2]);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		printf("%s\n", gai_strerror(rv));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
			perror("client: connect");
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

	send(sockfd, username, strlen(username) + 1, 0);

	if (!(pid = fork()))
		recv_loop(sockfd);
	
	printf(PROMPT);
	while ((read = getline(&line, &len, stdin)) > 0){
		x = line;
		while (isspace(*x ))
			x++;
		if (strncmp(x, "write", 5) == 0){
			msglen = strlen(x + 5);
			if (msglen < MAXDATA) { /* can fit into one packet */
				memcpy(sendbuff, "write", 5);
				set_int(1, sendbuff + 5);
				memcpy(sendbuff + 5 + sizeof(int), x + 5, msglen);
				tcp_send(sockfd, sendbuff, msglen + 5 + sizeof(int));
			} else { /* break it up into packets and send */
				for ( i = 0; i < (msglen / MAXDATA); i++) {
					memcpy(sendbuff, "write", 5);
					set_int(i + 1, sendbuff + 5);
					memcpy(sendbuff + 5 + sizeof(int), x + 5 + (MAXDATA*i), MAXDATA);
					tcp_send(sockfd, sendbuff, MAXDATA + 5 + sizeof(int));
				}
				memcpy(sendbuff, "write", 5);
				set_int(i + 1, sendbuff + 5);
				memcpy(sendbuff + 5 + sizeof(int), x + 5 + (MAXDATA*i), msglen % MAXDATA);
				tcp_send(sockfd, sendbuff, (msglen % MAXDATA) + 5 + sizeof(int));
				
			}
		//	send(sockfd, x, strlen(x), 0);
		}
		else if (strncmp(x, "put", 3) == 0) {
			/*send(sockfd, x, strlen(x), 0);
			send_file(sockfd, x + 3);*/
			kill(pid, SIGKILL);

			x += 3;
			while (isspace(*x))
				x++;

			if (strlen(x) > 0) {
				x[strlen(x) - 1] = '\0'; /* getline adds a newline, delete it */
				do_put(x, sockfd);
			} else {
				printf("enter a non-empty filename\n");
			}

			if ((pid = fork()) == 0)
				recv_loop(sockfd);

		}
		else if (strncmp(x, "list", 4) == 0) {
			tcp_send(sockfd, x, strlen(x));
		}
		else if (strncmp(x, "get", 3) == 0) {
			kill(pid, SIGKILL);
			/*send(sockfd, x, strlen(x), 0);
			get_file(sockfd);*/

			x += 3;
			while (isspace(*x))
				x++;

			if (strlen(x) > 0) {
				x[strlen(x) - 1] = '\0'; /* getline adds a newline, delete it */
				do_get(x, sockfd);
			} else {
				printf("enter a non-empty filename\n");
			}

			if ((pid = fork()) == 0)
				recv_loop(sockfd);
		}
		else
			usage();
		//send(sockfd, line, strlen(line), 0);
		printf(PROMPT);
	}
	free(line);
}
