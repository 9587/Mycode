#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arap/inet.h>
#include <pthread.h>
#include <fcntl.h>

#define BUFFLEN 1024		//缓冲区长度
#define NAMELEN 20			//名字长度

typedef struct dataHdr
{
	char flag;
	char user[20];
}DATA;

void *keepAlive(void *arg);

int main(int argc, char *argv[])
{
	int sockfd, retval, maxfd;
	char sendbuf[BUFFLEN] = {0};
	char recvbuf[BUFFLEN] = {0};
	struct sockaddr_in serverAddr;	//服务器地址
	struct sockaddr_in destaddr;	//目标地址，传输文件用
	
	pthread_t tid1, tid2;
	
	fd_set readset;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
		err_sys("socket error");
	
	bzero(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9999);
	inet_pton(AF_IENT, "127.0.0.1", &serverAddr.sin_addr.s_addr);
	
	/* 与服务器建立连接 */
	retval = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if(retval != 0)
		err_sys("connect error!");
	
	/* 处理保活函数 */
	retval = pthread_create(&tid1, NULL, keepAlive, (void *)sockfd);
	if(retval == -1)
		err_sys("pthread_create error");
	pthread_detach(tid1);
	
	char name[NAMELEN] = {0};
	int recvlen = 0;
	maxfd = sockfd + 1;
	
	/* 注册帐号 */
	while(1)
	{
		/* 提示注册 */
		printf("[Log in]please input your username:");
		fgets(name, sizeof(name), stdin);	//注册的用户名
		
		sendbuf[0] = 'R';			//服务器用来识别的标志位
		strcat(sendbuf, name);
		
		/* 给服务器发送注册信息(R + 注册名) */
		retval = send(sockfd, sendbuf, strlen(sendbuf), 0);
		if(retval == -1)
			err_sys("send error");
		
		/* 接收服务返回的信息, 判断是否注册成功 */
		recvlen = recv(sockfd, recvbuf, BUFFLEN, 0);
		if(recvlen == -1)
			err_sys("recv error");
		
		printf("%s\n", recvbuf + 1);
		
		/* A : 注册成功 */
		if(*recvbuf == 'A')
			break;
		
		/* E : 注册失败,帐号已存在 */
		/* 继续注册或者退出客户端 */
		if(*recvbuf == 'E')
		{
			char ch = 0;
			printf("Continue to register or exit?(reg->y / exit->n)\n");
			scanf("%c", &ch);
			
			if(ch == 'y')
				continue;
			else
				return 0;
		}
	}
	
	/* 聊天开始() */
	while(1)
	{
		char message[BUFFLEN] = {0};
		
		/* 查找聊天对象是否在线 */
		while(1)
		{
			FD_ZERO(0, &readset);
			FD_SET(sockfd, &readset);
			FD_SET(0, &readset);
			
			bzero(name, NAMELEN);
			bzero(sendbuf, BUFFLEN);
			
			retval = select(maxfd, &readset, NULL, NULL, NULL);
			if(retval == -1)
				err_sys("select error");
			
			/* 给服务器发消息 */
			if(FD_ISSET(0, &readset))
			{
				/* 输入聊天对象的用户名 */
				fgets(name, sizeof(name), stdin);
				sendbuf[0] = 'U';
				strncat(sendbuf, name, strlen(name));
				retval = send(sockfd, sendbuf, strlen(sendbuf), 0);
				if(retval == -1)
					err_sys("send error");
			}
			printf("who do you want to chat with?\n");
		
			if(FD_ISSET(sockfd, &readset))
			{
				recvlen = recv(sockfd, recvbuf, BUFFLEN, 0);
				if(recvlen == -1)
					err_sys("recv error!");
			
				printf("%s\n", recvbuf + 1);
				
				/* B不在线 */
				if(*recvbuf == 'E')
				{
					//printf("%s\n", recvbuf + 1);
					//continue;
				}
			
				/* B在线*/
				if(*recvbuf == 'A')
				{
					//printf("%s\n", recvbuf + 1);
					break;
				}
			
				/* 寻找聊天对象过程中,陌生人想和你聊天(可以做拒绝和接受功能) */
				// if(*recvbuf == 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXX')
				// {
					// 将陌生人的的名字拷给name
					// break;
				// }
			
				/* 寻找聊天对象过程中,陌生人想给你发文件(可以做拒绝和接受功能)*/
				// if(*recvbuf == 'XXXXXXXXX')
				// {
					/* 默认接收, 开一个线程接收文件 */
				// }
			}
		}
		
		char message[BUFFLEN] = {0};
		
		/* 和B开始聊天 */
		while(1)
		{
			bzero(sendbuf, BUFFLEN);
			sendbuf[0] = 'U';			//服务器识别信息的标志位
			strncat(sendbuf, NAMELEN);	//B的名字
			
			FD_ZERO(0, &readset);
			FD_SET(sockfd, &readset);
			FD_SET(0, &readset);
			
			retval = select(maxfd, &readset, NULL, NULL, NULL);
			if(retval == -1)
				err_sys("select error");
			
			/* 给服务器发送数据 */
			if(FD_ISSET(0, &readset))
			{
				fgets(message, BUFFLEN, stdin);
				
				/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
				strcpy(sendbuf + 21, message);
				retval = send(sockfd, sendbuf, 21 + strlen(message), 0);
				if(retval == -1)
					err_sys("send error");
			}
			
			/* 接收来自服务器的消息 */
			if(FD_ISSET(socket, &readset))
			{
				bzero(recvbuf, BUFFLEN);
				recvlen = recv(sockfd, recvbuf, BUFFLEN, 0);				
				if(recvlen == -1)
					err_sys("recv error");
				if(recvlen == 0)
				{
					printf("server close");
					//exit()
					break;
				}
				
				/* B已经下线 */
				//if(*recvbuf == 'E')
				//{
					//printf("%s\n", recvbuf + 1);
					//continue;
				//}
				
				/*B没有收到你的消息 */
				//if(*recvbuf == '')
				//{
					//
				//}
				
				/* B收到消息 */
				//if(*recvbuf == 'A')
				//{
					//printf("%s\n", recvbuf + 1);
				//}
				
				/* B给你发消息 */
				//if(*recvbuf == '')
				//{
					/* 接收B的消息内容 */
				//}
				
				/* 聊天过程中,某老王想和你聊天(可以做拒绝和接受功能) */
				//if(*recvbuf == 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXX')
				//{
					//将老王的名字拷给name
				//}
			}
		}
	}
}

void *keepAlive(void *arg)
{
	int fd = (int)arg;
	
	return NULL;
}