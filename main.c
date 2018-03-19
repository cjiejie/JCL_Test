/*
 * main.c
 *      function: 
 *      Created on: 2018��3��12��
 *      Author: cjiejie
 *     Mail: cjiejie@outlook.com  
 */

#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include<sys/un.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <pwd.h>

#include "../JCL/socket/socket.h"
#include "../JCL/io/io.h"
#include "../JCL/JCL.h"

#define AUTHOR "cjiejie"

#define TEST_TCP_SERVER		"tcp_ser"
#define TEST_TCP_CLI			"tcp_cli"
#define TEST_IO_READ			"io_read"
#define TEST_IO_SEND			"io_send"

#define TEST_IP			"172.16.34.116"
#define TEST_PORT		9896

void UsagePrint()
{
	printf("====================\n");
	printf("Usage: test <option> ...\n");
	printf("\t-h\t\t--help\t\toutput usage and exit.\n");
	printf("\t-d \t\t--debug\t\tdebug level.\n");
	printf("\t-t \t\t--test\t\ttest mode, with : %s,%s,%s,%s.\n",TEST_TCP_SERVER,TEST_TCP_CLI,\
					TEST_IO_READ,TEST_IO_SEND);
	printf("\t-v\t\t--version\tshow version and exit.\n");
	printf("====================\n");
}

int TestTcpServer()
{
	printf("<%s,%d>into...\r\n",__func__,__LINE__);
	int sock_fd = CreatTCPSocketServer(TEST_IP,TEST_PORT);
	if(sock_fd <3) {
		printf("<%s,%d>Creat tcp err,ret[%d]\r\n",__func__,__LINE__,sock_fd);
		return -1;
	}
	printf("<%s,%d>Creat tcp OK %s:%d\r\n",__func__,__LINE__,TEST_IP,TEST_PORT);
	unsigned int cliaddr_len = 0;
	int connfd = -1,ret = -1;
	struct sockaddr_in cli_addr = {0};
	char buf[20]  = {0};
	char send_buf[] = "200";
	char remote_ip[30] = {0};
	int read_len = -1;
	while(1) {
		cliaddr_len = sizeof(cli_addr);
		connfd = accept(sock_fd, (struct sockaddr *)&cli_addr, &cliaddr_len);
		if(connfd <3) {
			printf("<%s,%d>accept tcp err,ret[%d]\r\n",__func__,__LINE__,connfd);
			return -1;
		}
		inet_ntop(AF_INET, &cli_addr.sin_addr, remote_ip, 16);
		printf("connect from %s:%d\n",remote_ip , ntohs(cli_addr.sin_port));
		while(1) {
			memset(buf,0,sizeof(buf));
			read_len = ReadSocket(connfd, buf, sizeof(buf),1000);
			if(read_len > 0) {
				printf("<%s,%d>read:%d,%s \r\n",__func__,__LINE__,read_len,buf);
				ret = WriteSocket(connfd,send_buf,strlen(send_buf),1000);
				if(ret < 0) {
					printf("<%s,%d> select err: errno[%d], %s\n",__func__,__LINE__,errno, strerror(errno));
				}
				if(NULL != strstr(buf,"end")) {
					printf("<%s,%d>disconnect... \r\n",__func__,__LINE__);
					close(connfd);
					break;
				}
			} else if(read_len < 0) {
				printf("<%s,%d>disconnect... \r\n",__func__,__LINE__);
				close(connfd);
				break;
			}
			sleep(1);
		}
	}
}

int TestTcpCli()
{
	printf("<%s,%d>into...!\r\n",__func__,__LINE__);
	int ret = -1;
	char read_buf[100] = {0};
	char send_buf[] = "cccccccccc";
	int fd = ConnectTCPSocketServer(TEST_IP,TEST_PORT);
	if(fd < 3) {
		printf("<%s,%d>connect err!ret[%d]\r\n",__func__,__LINE__,fd);
		return -1;
	}
	printf("<%s,%d>connect tcp OK %s:%d\r\n",__func__,__LINE__,TEST_IP,TEST_PORT);
	while(1) {
		memset(read_buf,0,sizeof(read_buf));
		WriteSocket(fd,send_buf,strlen(send_buf),1000);
		ret = ReadSocket(fd,read_buf,sizeof(read_buf),1000);
		if(ret > 0) {
			printf("<%s,%d>read:%d,%s \r\n",__func__,__LINE__,ret,read_buf);
		} else if(ret < 0) {
			printf("<%s,%d>disconnect, try again...\r\n",__func__,__LINE__);
			close(fd);
			sleep(1);
			fd = ConnectTCPSocketServer(TEST_IP,TEST_PORT);
			if(fd < 3) {
				printf("<%s,%d>connect err!ret[%d]\r\n",__func__,__LINE__,fd);
				return -1;
			}
			break;
		}
		sleep(1);
	}

	return 0;
}

int TestIOSend()
{
	printf("<%s,%d>not support!\r\n",__func__,__LINE__);
	return 0;
}

int TestIORead()
{
	printf("<%s,%d>not support!\r\n",__func__,__LINE__);
	return 0;
}

int TestFunc(char * func)
{
	int ret = -1;
	if(0 == strcmp(func,TEST_TCP_SERVER)) {
		ret = TestTcpServer();
	} else if(0 == strcmp(func,TEST_TCP_CLI)) {
		ret = TestTcpCli();
	} else if(0 == strcmp(func,TEST_IO_READ)) {
		ret = TestIOSend();
	} else if(0 == strcmp(func,TEST_IO_SEND)) {
		ret = TestIORead();
	} else {
		printf("<%s,%d>not support!\r\n",__func__,__LINE__);
	}
	if(-1 == ret) {
		printf("<%s,%d>test err!\r\n",__func__,__LINE__);
	}
	return ret;
}

int main(int argc, char *argv[])
{
	int debug_level = 0;
	int next_option = -1;
	int verbose = 0;
	/* An array describing valid long options. */
	const struct option long_options[] = {
	   {"help", 0, NULL, 'h'},
	   {"debug", 1, NULL, 'd'},
	   {"version", 0, NULL, 'v'},
	   {"test", 0, NULL, 't'},
	   {NULL, 0, NULL, 0}  /* Required at end of array. */
	};
	 /* A string listing valid short options letters. */
	const char* const short_options = "hd:vt:";
	do {
		 next_option = getopt_long (argc, argv, short_options, long_options, NULL);
		 switch(next_option) {
			case 'd':    /* -o or --output */
				debug_level = atoi(optarg);
				break;
			case -1:    /* Done with options. */
				break;
			case 'v':    /* -v or --version */
				verbose = 1;
				break;
			 case 'h':    /* -h or --help */
				 UsagePrint();
				 return 0;
			 case 't':
				 TestFunc(optarg);
				 return 0;
			case '?':    /* The user specified an invalid option. */
			default:    /* Something else: unexpected. */
				UsagePrint();
				return 0;
			 }
		} while (next_option != -1);
	if(1 == verbose) {
		struct passwd *pwd;
		pwd = getpwuid(getuid());
		printf("====================\n");
		printf("%s run by [%s],built by %s in [%s %s]\n",argv[0], pwd->pw_name,AUTHOR,__DATE__,__TIME__);
		LibBuiltTime();
		printf("====================\n");
		return 0;
	}
	printf("debug_level:%d. \r\n",debug_level);
	printf("Built by %s in [%s %s]\n",AUTHOR,__DATE__,__TIME__);
	return 0;
}





































