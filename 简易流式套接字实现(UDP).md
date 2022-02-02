> socket()套接字编程练习，这里实现的是UDP，也就是AF_INET族的SOCK_DGRAM，默认0即UDP协议

### proto.h
```cpp
#ifndef PROTO_H__
#define PROTO_H__
#define NAMESIZE 11
#define RCVPORT 1937
#include<stdint.h>
//定义接受方端口(最好>1024
struct msg_st
{
	uint8_t name[NAMESIZE]; //统一类型标准
	uint32_t math;
	uint32_t chinese;
}__attribute__((packed)); //不对齐

#endif
```
* 封装消息结构体，并且规定接收方(**被动端**)的端口号
* 因为被动端首先运行，在bind套接字绑定时**必须指定监听端口**。
* 而发送方的socket绑定端口其实**可有可无**，因为sendto发送时机器会**自动分配一个指定**。

### recv.c
```cpp
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include "proto.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: argc < 2\n");
		exit(1);		
	}
	int sfd;
	struct msg_st sbuf;
	struct sockaddr_in raddr;
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sfd < 0)
	{
		perror("socket()");
		exit(1);
	}
	strcpy(sbuf.name, "Zhang Huan");
	sbuf.math = htonl(rand() % 100);
	sbuf.chinese = htonl(rand() % 100);

	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(RCVPORT); //设置对面信息
	inet_pton(AF_INET, argv[1], &raddr.sin_addr);
	
	//sendto用于报式传输 send用于流式传输
	if(sendto(sfd, &sbuf, sizeof(sbuf), 0, (void *)&raddr, sizeof(raddr)) < 0)
	{
		perror("sendto()");
		exit(1);
	}
	puts("Send OK!");
	close(sfd);
	exit(0);
}
```
* 这里实现被动端，socket之后返回一个**文件描述符**(万物皆文件嘛)，然后bind绑定端口和IP(0.0.0.0为本机IP)，之后recvfrom接收消息并且**记录发送者**的端口和地址信息。
* `./recv`之后阻塞等待，可以通过`netstat -anu`查看，可以看到第一个UDP连接已经建立，端口为指定的1937：

![image](https://user-images.githubusercontent.com/55400137/152160242-76f21ab3-5ee3-4aa0-9093-11cee52b33b8.png)


### send.c
```cpp
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include "proto.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: argc < 2\n");
		exit(1);		
	}
	int sfd;
	struct msg_st sbuf;
	struct sockaddr_in raddr;
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sfd < 0)
	{
		perror("socket()");
		exit(1);
	}
	strcpy(sbuf.name, "Zhang Huan");
	sbuf.math = htonl(rand() % 100);
	sbuf.chinese = htonl(rand() % 100);

	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(RCVPORT); //设置对面信息
	inet_pton(AF_INET, argv[1], &raddr.sin_addr);
	
	//sendto用于报式传输 send用于流式传输
	if(sendto(sfd, &sbuf, sizeof(sbuf), 0, (void *)&raddr, sizeof(raddr)) < 0)
	{
		perror("sendto()");
		exit(1);
	}
	puts("Send OK!");
	sleep(20);
	close(sfd);
	exit(0);
}
```
* 发送方也是socket一个套接字，之后我们**不显示绑定**端口，在命令行接收对面的IP信息直接sendto给它，由于我们接收方也在本机，因此可以0.0.0.0，也可以127.0.0.1
* 执行命令`./send 0.0.0.0`或者`./send 127.0.0.1`之后，将发送消息，可以看到成功接收，而且由于没有**显式**bind绑定端口，每次发送时发送者的端口都是**随机分配**的：

![image](https://user-images.githubusercontent.com/55400137/152160871-3ba1fef2-2a38-422b-9a62-9878e4c066a9.png)

呃这样一个简单的UDP报式传输套接字就实现了。
