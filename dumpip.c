/* 
 * dumpip  - A simple TCP server to dump client IP
 * usage: dumpip <port> [ logfile ]
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024

void error(char *msg)
{
	perror(msg);
	exit(1);
}

void daemon_init(void)
{
	int pid;
	pid = fork();
	if (pid == -1)
		error("fork error!");
	else if (pid > 0)
		exit(0);

	if (setsid() == -1)
		error("setsid error!");

	pid = fork();
	if (pid == -1)
		error("fork error!");
	else if (pid > 0)
		exit(0);
	setsid();
	close(0);
	close(1);
	close(2);
}

int main(int argc, char **argv)
{
	int listenfd;
	int childfd;
	int port;
	int ipv6 = 0;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	char buf[BUFSIZE];
	int optval;

	if (argc < 2) {
		fprintf(stderr, "usage: %s [ -6 ] <port> [ logfile ]\n", argv[0]);
		exit(1);
	}

	daemon_init();

	if (strcmp(argv[1], "-6") == 0)
		ipv6 = 1;
	port = atoi(argv[1 + ipv6]);

	if (ipv6)
		listenfd = socket(AF_INET6, SOCK_STREAM, 0);
	else
		listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0)
		error("ERROR opening socket");

	optval = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
	if (ipv6) {
		struct sockaddr_in6 serveraddr6;
		bzero((char *)&serveraddr6, sizeof(serveraddr6));
		serveraddr6.sin6_family = AF_INET6;
		serveraddr6.sin6_port = htons((unsigned short)port);
		if (bind(listenfd, (struct sockaddr *)&serveraddr6, sizeof(serveraddr6)) < 0)
			error("ERROR on binding");
	} else {
		struct sockaddr_in serveraddr;
		bzero((char *)&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serveraddr.sin_port = htons((unsigned short)port);
		if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
			error("ERROR on binding");
	}

	if (listen(listenfd, 5) < 0)
		error("ERROR on listen");

	clientlen = sizeof(clientaddr);
	while (1) {
		char hbuf[INET6_ADDRSTRLEN];
		childfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		if (childfd < 0)
			continue;
		if (clientaddr.ss_family == AF_INET6) {
			struct sockaddr_in6 *r = (struct sockaddr_in6 *)&clientaddr;
			inet_ntop(AF_INET6, &r->sin6_addr, hbuf, sizeof(hbuf));
			if (memcmp(hbuf, "::ffff:", 7) == 0)
				strcpy(hbuf, hbuf + 7);

			port = ntohs(r->sin6_port);
		} else {
			struct sockaddr_in *r = (struct sockaddr_in *)&clientaddr;
			inet_ntop(AF_INET, &r->sin_addr, hbuf, sizeof(hbuf));
			port = ntohs(r->sin_port);
		}
		snprintf(buf, BUFSIZE, "\n\n\nYour IP is %s\n\nYour Port is %d\n\n", hbuf, port);

		write(childfd, buf, strlen(buf));

		if (argc >= 3 + ipv6) {
			FILE *fp;
			fp = fopen(argv[2 + ipv6], "a");
			time_t nowt;
			struct tm *c_tm;
			nowt = time(NULL);
			c_tm = localtime(&nowt);
			fprintf(fp, "%d-%02d-%02d %02d:%02d:%02d %s\n",
				c_tm->tm_year + 1900, c_tm->tm_mon + 1,
				c_tm->tm_mday, c_tm->tm_hour, c_tm->tm_min, c_tm->tm_sec, hbuf);
			fclose(fp);
		}

		close(childfd);
	}
}
