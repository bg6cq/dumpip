/*
 * dumpip  - A simple TCP server to dump client IP, log to file or send to a url
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

#define MAXLEN 2048

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

void usage(void)
{
	printf("dumpip: A simple TCP server to dump client IP, log to file or send to a url\n\n");
	printf
	    ("dumpip [ -d ] [ -6 ] [ -k apikey_file ] [ -h ] [ -l logfile ] [ -u report_url ] [ -r reportip ] -p port\n\n");
	printf("   -p port            listen port\n\n");
	printf("   -d                 enable debug\n");
	printf("   -6                 enable ipv6\n");
	printf("   -l logfile         logfile path\n");
	printf("   -k apikei_keyfile  keyfile, default is apikey.txt\n");
	printf
	    ("   -u report_url      report_url, such as http://blackip.ustc.edu.cn/portscan.php\n");
	printf("   -r reportip        report ip\n\n");
	printf("report method POST:\n");
	printf("   curl -d @- report_url\n");
	printf
	    ("   POST data    apikey=key&reportip=IP&fromip=IP&fromport=FROMPORT&toip=TOIP&toport=TOPORT\n\n");
	exit(0);
}


char apikey[MAXLEN];
char logfile[MAXLEN];
char report_url[MAXLEN];
char reportip[MAXLEN];
int debug = 0;

static unsigned char hexchars[] = "0123456789ABCDEF";

int URLEncode(const char *str, const int strsz, char *result, const int resultsz)
{
	int i, j;
	char ch;

	if (strsz < 0 || resultsz < 0)
		return -1;

	for (i = 0, j = 0; i < strsz && j < resultsz; i++) {
		ch = *(str + i);
		if ((ch >= 'A' && ch <= 'Z') ||
		    (ch >= 'a' && ch <= 'z') ||
		    (ch >= '0' && ch <= '9') || ch == '.' || ch == '-' || ch == '*' || ch == '_')
			result[j++] = ch;
		else if (ch == ' ')
			result[j++] = '+';
		else {
			if (j + 3 <= resultsz - 1) {
				result[j++] = '%';
				result[j++] = hexchars[(unsigned char)ch >> 4];
				result[j++] = hexchars[(unsigned char)ch & 0xF];
			} else {
				return -2;
			}
		}
	}

	if (i == 0) {
		result[0] = 0;
		return 0;
	} else if (i == strsz) {
		result[j] = 0;
		return j;
	}
	result[0] = 0;
	return -2;
}

void readkey(char *fname)
{
	FILE *fp;
	char buf[MAXLEN];
	fp = fopen(fname, "r");
	if (fp == NULL)
		return;
	if (fgets(buf, MAXLEN, fp)) {
		if (strlen(buf) <= 1) {
			fclose(fp);
			return;
		}
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = 0;
		strncpy(apikey, buf, MAXLEN);
	}
	fclose(fp);
}

int fileexists(const char *filename)
{
	FILE *file;
	file = fopen(filename, "r");
	if (file != NULL) {
		fclose(file);
		return 1;
	}
	return 0;
}


int main(int argc, char **argv)
{
	int listenfd;
	int childfd;
	int port = 0, myport;
	int ipv6 = 0;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	int optval;
	char buf[MAXLEN];
	int c;
	while ((c = getopt(argc, argv, "dhk:l:u:r:p:")) != EOF)
		switch (c) {
		case 'd':
			debug = 1;
			break;
		case '6':
			ipv6 = 1;
			break;
		case 'h':
			usage();
		case 'k':
			readkey(optarg);
			break;
		case 'l':
			strncpy(logfile, optarg, MAXLEN);
			break;
		case 'u':
			strncpy(report_url, optarg, MAXLEN);
			break;
		case 'r':
			strncpy(reportip, optarg, MAXLEN);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		}
	if (report_url[0] != 0) {
		if (apikey[0] == 0)
			readkey("apikey.txt");
		if (apikey[0] == 0) {
			fprintf(stderr, "ERROR: apkey = NULL\n\n");
			usage();
		}
	}
	if (port == 0)
		usage();


	if (debug) {
		printf("apikey: %s\n", apikey);
		printf("logfile: %s\n", logfile);
		printf("report_url: %s\n", report_url);
		printf("reportip: %s\n", reportip);
		printf("listen port: %d\n", port);
		printf("IPv6: %s\n", ipv6 == 1 ? "yes" : "no");
	} else
		daemon_init();

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
		socklen_t mylen;
		struct sockaddr_storage myaddr;
		char hbuf[INET6_ADDRSTRLEN];
		char mybuf[INET6_ADDRSTRLEN];
		childfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		if (childfd < 0)
			continue;
		mylen = sizeof(myaddr);
		getsockname(childfd, (struct sockaddr *)&myaddr, &mylen);
		if (clientaddr.ss_family == AF_INET6) {
			struct sockaddr_in6 *r = (struct sockaddr_in6 *)&clientaddr;
			inet_ntop(AF_INET6, &r->sin6_addr, hbuf, sizeof(hbuf));
			if (memcmp(hbuf, "::ffff:", 7) == 0)
				strcpy(hbuf, hbuf + 7);

			port = ntohs(r->sin6_port);

			r = (struct sockaddr_in6 *)&myaddr;
			inet_ntop(AF_INET6, &r->sin6_addr, mybuf, sizeof(mybuf));
			if (memcmp(mybuf, "::ffff:", 7) == 0)
				strcpy(mybuf, mybuf + 7);
			myport = ntohs(r->sin6_port);
		} else {
			struct sockaddr_in *r = (struct sockaddr_in *)&clientaddr;
			inet_ntop(AF_INET, &r->sin_addr, hbuf, sizeof(hbuf));
			port = ntohs(r->sin_port);
			r = (struct sockaddr_in *)&myaddr;
			inet_ntop(AF_INET, &r->sin_addr, mybuf, sizeof(mybuf));
			myport = ntohs(r->sin_port);
		}
		snprintf(buf, MAXLEN,
		    "\n\n\nYour IP is %s\n\nYour Port is %d\n\nMy IP is: %s\n\nMy Port is %d\n\n",
			 hbuf, port, mybuf, myport);

		write(childfd, buf, strlen(buf));

		if (logfile[0]) {
			FILE *fp;
			time_t nowt;
			struct tm *c_tm;
			fp = fopen(argv[2 + ipv6], "a");
			if (fp) {
				nowt = time(NULL);
				c_tm = localtime(&nowt);
				fprintf(fp, "%d-%02d-%02d %02d:%02d:%02d %s %s %d %d\n",
					c_tm->tm_year + 1900, c_tm->tm_mon + 1,
				   c_tm->tm_mday, c_tm->tm_hour, c_tm->tm_min, c_tm->tm_sec, hbuf,
					mybuf, port, myport);
				fclose(fp);
			}
		}
		close(childfd);
		if (report_url[0] == 0)
			continue;
		snprintf(buf, MAXLEN, "curl -d @- %s", report_url);
		FILE *fpcmd;
		fpcmd = popen(buf, "w");
		if (fpcmd == NULL) {
			if (debug)
				fprintf(stderr, "popen %s error\n", buf);
			continue;
		}
		fprintf(fpcmd,
			"apikey=%s&reportip=%s&fromip=%s&fromport=%d&toip=%s&toport=%d",
			apikey, reportip, hbuf, port, mybuf, myport);
		if (debug)
			fprintf(stderr,
				"apikey=%s&reportip=%s&fromip=%s&fromport=%d&toip=%s&toport=%d",
				apikey, reportip, hbuf, port, mybuf, myport);
		pclose(fpcmd);
	}
}
