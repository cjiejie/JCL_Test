/*
 * main.c
 *      function: 
 *      Created on: 2018年3月12日
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

#define TEST_UDP_SERVER		"udp_ser"
#define TEST_UDP_CLI			"udp_cli"

#define TEST_LOCAL_SER		"local_ser"
#define TEST_LOCAL_CLI		"local_cli"

#define TEST_IO_READ			"io_read"
#define TEST_IO_SEND			"io_send"

#define TEST_IP			"172.16.34.116"
#define TEST_PORT		9896

#define MAX_LEN 1024
#define HALF_LEN 512

#define TEST_LOCAL_SOCK_NAME	"/tmp/test_socket_name"

void UsagePrint()
{
	printf("========================================\n");
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	printf("Usage: test [option] [func] <param>\n");
	printf(" -h\t--help\t\tOutput usage and exit.\n");
	printf(" -d\t--debug\t\tDebug level.\n");
	printf(" -v\t--version\tShow version and exit.\n");
	printf(" -t \t--test\t\tTest mode, with : %s,%s;\n \t\t\t%s,%s,%s,%s;\n"
					" \t\t\t%s,%s\n",TEST_TCP_SERVER,TEST_TCP_CLI,\
					TEST_IO_READ,TEST_IO_SEND,TEST_LOCAL_SER,TEST_LOCAL_CLI,
					TEST_UDP_SERVER,TEST_UDP_CLI);
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	printf("========================================\n");
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

int TestUdpServer()
{
	printf("<%s,%d>into...!\r\n",__func__,__LINE__);
	int socket_fd = -1,count = -1;
	socket_fd = CreatUDPSocketServer(TEST_IP,TEST_PORT,5000);
	if(socket_fd < 0) {
		printf("<%s,%d>jcl: Creat UDP ser err!\r\n",__func__,__LINE__);
		return -1;
	}
	char buf[HALF_LEN] = {0};
	char send_buf[] = "200";
	struct sockaddr_in client_addr = {0};  //clent_addr用于记录发送方的地址信息
	socklen_t len = sizeof(client_addr);
	while(1) {
		count = recvfrom(socket_fd, buf, sizeof(buf), 0, (struct sockaddr*)&client_addr, &len);
		if(count == -1 && errno == EAGAIN) {
			printf("<%s,%d>jcl: recvfrom timeout, continue...\r\n",__func__,__LINE__);
			continue;
		} else if(count > 0) {
			printf("<%s,%d>form:%s:%d \r\n",__func__,__LINE__,inet_ntoa(client_addr.sin_addr),client_addr.sin_port);
			printf("<%s,%d>read:%d,%s \r\n",__func__,__LINE__,count,buf);
			//发送信息给client，注意使用了clent_addr结构体指针
			count = sendto(socket_fd, send_buf, sizeof(send_buf), 0, (struct sockaddr*)&client_addr, len);
			if(count < 0) {
				printf("<%s,%d> sendto err: errno[%d], %s\n",__func__,__LINE__,errno, strerror(errno));
				printf("<%s,%d>continue... \r\n",__func__,__LINE__);
				continue;
			}
		} else {
			printf("<%s,%d> recvfrom err: errno[%d], %s\n",__func__,__LINE__,errno, strerror(errno));
			close(socket_fd);
			return -1;
		}
	}

	return 0;
}

int TestUdpCli()
{
	printf("<%s,%d>into...!\r\n",__func__,__LINE__);
	int socket_fd = -1,count = -1;
	struct sockaddr_in ser_addr = {0};
	socket_fd = ConnectUDPSocketServer(TEST_IP,TEST_PORT,(struct sockaddr*)&ser_addr);
	if(socket_fd < 0) {
		printf("<%s,%d>jcl: Connect UDP ser err!\r\n",__func__,__LINE__);
		return -1;
	}
	socklen_t len = 0;
	char read_buf[100] = {0};
	char send_buf[] = "cccccccccc";
	while(1) {
		len = sizeof(struct sockaddr_in);
		count = sendto(socket_fd, send_buf, sizeof(send_buf), 0, (struct sockaddr*)&ser_addr, len);
		if(count <= 0) {
			printf("<%s,%d> sendto err: errno[%d], %s\n",__func__,__LINE__,errno, strerror(errno));
			printf("<%s,%d>continue... \r\n",__func__,__LINE__);
			sleep(3);
			continue;
		} else {
			printf("<%s,%d>send ok. \r\n",__func__,__LINE__);
			count = recvfrom(socket_fd, read_buf, sizeof(read_buf), 0, (struct sockaddr*)&ser_addr, &len);  //接收来自server的信息
			if(count == -1 && errno == EAGAIN) {
				printf("<%s,%d>jcl: recvfrom timeout, continue...\r\n",__func__,__LINE__);
				continue;
			} else if(count > 0) {
				printf("<%s,%d>read:%d,%s \r\n",__func__,__LINE__,count,read_buf);
				sleep(1);
			}
		}
	}
	return 0;
}

int TestLocalSer()
{
	printf("<%s,%d>into...\r\n",__func__,__LINE__);
	int sock_fd = CreatLocalSocketServer(TEST_LOCAL_SOCK_NAME,I_AUTO);
	if(sock_fd <3) {
		printf("<%s,%d>Creat local sock err,ret[%d]\r\n",__func__,__LINE__,sock_fd);
		return -1;
	}
	printf("<%s,%d>Creat local sock OK.\r\n",__func__,__LINE__);
	unsigned int cliaddr_len = 0;
	int connfd = -1,ret = -1;
	struct sockaddr_in cli_addr = {0};
	char buf[20]  = {0};
	char send_buf[] = "200";
	int read_len = -1;
	while(1) {
		cliaddr_len = sizeof(cli_addr);
		connfd = accept(sock_fd, (struct sockaddr *)&cli_addr, &cliaddr_len);
		if(connfd <3) {
			printf("<%s,%d>accept local sock err,ret[%d]\r\n",__func__,__LINE__,connfd);
			return -1;
		}
		printf("local sock connect.\n");
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

int TestLocalCli()
{
	printf("<%s,%d>into...!\r\n",__func__,__LINE__);
	int ret = -1;
	char read_buf[100] = {0};
	char send_buf[] = "cccccccccc";
	int fd = ConnectLocalSocket(TEST_LOCAL_SOCK_NAME);
	if(fd < 3) {
		printf("<%s,%d>connect err!ret[%d]\r\n",__func__,__LINE__,fd);
		return -1;
	}
	printf("<%s,%d>connect local sock OK.\r\n",__func__,__LINE__);
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
	} else if(0 == strcmp(func,TEST_LOCAL_SER)) {
		ret = TestLocalSer();
	} else if(0 == strcmp(func,TEST_LOCAL_CLI)) {
		ret = TestLocalCli();
	} else if(0 == strcmp(func,TEST_UDP_SERVER)) {
		ret = TestUdpServer();
	} else if(0 == strcmp(func,TEST_UDP_CLI)) {
		ret = TestUdpCli();
	} else {
		printf("<%s,%d>%s not support!\r\n",__func__,__LINE__,func);
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






































