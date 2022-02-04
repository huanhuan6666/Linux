> 用套接字编程实现一个流式套接字TCP，注意和报式套接字的区别

### proto.h


### server.h服务器端实现
* 服务器端的实现基本流程如下:
	* 取得socket
	* 将socket绑定地址bind
	* 将socket设置成监听模式listen
	* 接收连接请求accept
	* 收/发消息
	* 关闭socket
* 一些函数
```cpp
int listen(int sockfd, int backlog); //服务器端开始监听，backlog定义了sockfd上半连接队列的最大长度
//但是由于TCPcookie抛弃了半连接队列，这里我们通常认为是全连接队列的大小，即能够建立TCP连接的最大数目
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen); //接收半连接队列里的第一个连接请求，然后创建一个新的套接字与请求连接，并返回新的文件描述符
// 因为TCP连接是一对一的，而我们只给服务器指定了一个端口，难道就只能完成一个连接吗(毕竟一对一)？显然不合理，accept就是创建一个新的套接字去和请求连接，然后自己的套接字端口接着监听，合情合理
ssize_t send(int sockfd, const void *buf, size_t len, int flags);  //TCP的发送函数

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, //UDP的发送函数同样需要确定对面的接收者信息
                    const struct sockaddr *dest_addr, socklen_t addrlen);
```
这里再次说明一下send和sendto的区别，事实上`send(sockfd, buf, len, flags);`**等价于**`sendto(sockfd, buf, len, flags, NULL, 0);`，注意scokfd都是本进程的文件描述符，即**自己的套接字**，sendto需要指定对面地址因为UDP是无连接的；send不需要，因为TCP已经确定对面地址了。

服务器端代码如下：
```cpp
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "proto.h"

#define IPSTRSIZE 40
#define BUFSIZE 1024
void server_job(int fd)
{
	int len;
	char buf[BUFSIZE];
	len = sprintf(buf, FMT_STAMP, (long long)time(NULL));
	if(send(fd, buf, len, 0) < 0) //向套接字发内容
	{
		perror("send()");
		exit(1);
	}

}
int main()
{
	int sfd, newfd;
	struct sockaddr_in laddr, raddr;
	char ipstr[IPSTRSIZE];
	socklen_t rlen;
	sfd = socket(AF_INET, SOCK_STREAM, 0/*IPPROTO_TCP*/);//AF_INET协议族中SOCK_STREAM方式的默认协议就是TCP
	if(sfd < 0)
	{
		perror("socket()");
		exit(1);
	}
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
	if(bind(sfd, (void *)&laddr, sizeof(laddr)) < 0)//bind套接字绑定我S端的地址
	{
		perror("bind()");
		exit(1);
	}
	
	if(listen(sfd, 200) < 0) //服务器端开始监听
	{
		perror("listen()");
		exit(1);
	}
	rlen = sizeof(raddr);
	while(1)
	{
		newfd = accept(sfd, (void *)&raddr, &rlen); //接收连接并且记录发起连接的客户端地址
		if(newfd < 0)
		{

			perror("accept()");
			exit(1);
		}
		inet_ntop(AF_INET, &raddr.sin_addr, ipstr, IPSTRSIZE);
		printf("Clinet:%s:%d\n", ipstr, ntohs(raddr.sin_port));
		server_job(newfd);
		close(newfd);
	}
	
	close(sfd);
	exit(0);
}
```
之后`./server`启动服务器后，用`netstat -ant`查看所有TCP连接如下：

![image](https://user-images.githubusercontent.com/55400137/152514574-7cc6dcb0-ac95-466c-b89e-1788a4112ddb.png)

可以看到已经在listen了。可以使用`nc`命令或者`telnet`命令测试TCP端口：比如`nc 127.0.0.1 1937`.

nc命令**创建**了一个**TCP套接字**并向指定的127.0.0.1 1937端口发起连接：

![image](https://user-images.githubusercontent.com/55400137/152516875-ed769b6c-0ad1-4865-b87d-95c428f517b2.png)

这时`netstat -ant`可以看到nc命令创建的那个TCP套接字：

![image](https://user-images.githubusercontent.com/55400137/152517178-aa9e99a3-b6c5-4f77-878b-cd2f43f9352c.png)

服务器端也可以看到连接：

![image](https://user-images.githubusercontent.com/55400137/152517243-41fa73ad-7ad1-4f2f-8dc8-e10b56681ee4.png)




