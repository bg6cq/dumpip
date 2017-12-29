#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <endian.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <getopt.h>

#define MAXLEN 16384 

int debug=0;

#include "sock.h"

char * INET_NTOA(unsigned long int ip)
{ 	static char buf[100];
 	sprintf(buf,"%d.%d.%d.%d",
 	(unsigned int)((ip>>24)&0xff), (unsigned int)((ip>>16)&0xff), (unsigned int)((ip>>8)&0xff), (unsigned int)((ip)&0xff));
 	return buf;
}

char * INET_NTOA2(unsigned long int ip)
{ 	static char buf[100];
 	sprintf(buf,"%d.%d.%d.%d",
 	(unsigned int)((ip>>24)&0xff), (unsigned int)((ip>>16)&0xff), (unsigned int)((ip>>8)&0xff), (unsigned int)((ip)&0xff));
 	return buf;
}

void alarm_handler() {
	fprintf(stderr,"holdtime expired, exit\n");
	exit(0);
}

int do_dumpip(char *p, char *logf)
{
	unsigned char buf[MAXLEN];
	int lf;
	lf=Tcp_listen("0.0.0.0",p,NULL);
	
	while(1) {
		int s;
		int len;
		FILE *fp;
		struct sockaddr_in sa;
		len=sizeof(sa);
		s=Accept(lf,&sa,&len);
		snprintf(buf,MAXLEN,"\n\n\nYour IP is %s\n\n\n\n",INET_NTOA(ntohl(sa.sin_addr.s_addr)));
		write(s,buf,strlen(buf));
		close(s);
		fp=fopen(logf,"a");
		time_t nowt;
		struct tm * c_tm;
		nowt=time(NULL);
		c_tm = localtime(&nowt);
		fprintf(fp,"%d-%02d-%02d %02d:%02d:%02d %s\n",
			c_tm->tm_year+1900, c_tm->tm_mon+1,
                	c_tm->tm_mday, c_tm->tm_hour,
                	c_tm->tm_min, c_tm->tm_sec,
			INET_NTOA(ntohl(sa.sin_addr.s_addr)));
		fclose(fp);
		fprintf(stderr,"%d-%02d-%02d %02d:%02d:%02d %s\n",
			c_tm->tm_year+1900, c_tm->tm_mon+1,
                	c_tm->tm_mday, c_tm->tm_hour,
                	c_tm->tm_min, c_tm->tm_sec,
			INET_NTOA(ntohl(sa.sin_addr.s_addr)));
	}
	return 0;
}

void usage()
{
	fprintf(stderr,"Usage:  dumpip port log_file\n");
	exit(0);
}

int main(int argc, char*argv[])
{	int c;
	signal(SIGALRM,alarm_handler);
	if(argc!=3) usage();
	while(1) {
		int pid;
		pid=fork();
		if(pid==0) // child 
			return do_dumpip(argv[1],argv[2]);
		else if(pid==-1) // error
			exit(0);
		else 
			wait(NULL);
		sleep(10);
	}
}
