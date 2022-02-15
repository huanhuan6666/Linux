> 用套接字编程实现一个流式套接字TCP，注意和报式套接字的区别

### proto.h


### server.c服务器端实现
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
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen); 
//接收半连接队列里的第一个连接请求，然后创建一个新的套接字与请求连接，并返回新的文件描述符
// 因为TCP连接是一对一的，而我们只给服务器指定了一个端口，难道就只能完成一个连接吗(毕竟一对一)？
//显然不合理，accept就是创建一个新的套接字去和请求连接，然后自己的套接字端口接着监听，合情合理

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
* 服务器端socket获取套接字后，必须bind绑定端口和IP，因为是被动端，必须让别人知道自己在哪里
* 绑定好后开始listen监听客户端发来的连接请求即可并且会创建出**另一个套接字newfd**和请求连接，自己继续listen，accept只是从**完成队列**里取一个连接。
> 也就是说socket分为**监听型**套接字和**通信型**套接字，原本的socket只负责监听，listen三次握手时会建立另一个文件描述符**newfd绑定到同样的地址:端口上**，然后放到完成队列里，accept取时就会**返回**那个newfd，这就是服务器端**用于通信**的套接字。
* accept时服务器端能够获取到请求连接的客户端的IP和端口信息，我们的代码就是把该信息打印出来了。


之后`./server`启动服务器后，用`netstat -ant`查看所有TCP连接如下：

![image](https://user-images.githubusercontent.com/55400137/152514574-7cc6dcb0-ac95-466c-b89e-1788a4112ddb.png)

可以看到已经在listen了。可以使用`nc`命令或者`telnet`命令测试TCP端口：比如`nc 127.0.0.1 1937`.

nc命令**创建**了一个**TCP套接字**并向指定的127.0.0.1 1937端口发起连接：

![image](https://user-images.githubusercontent.com/55400137/152516875-ed769b6c-0ad1-4865-b87d-95c428f517b2.png)

这时`netstat -ant`可以看到nc命令创建的那个TCP套接字：

![image](https://user-images.githubusercontent.com/55400137/152517178-aa9e99a3-b6c5-4f77-878b-cd2f43f9352c.png)

服务器端也可以看到连接：

![image](https://user-images.githubusercontent.com/55400137/152517243-41fa73ad-7ad1-4f2f-8dc8-e10b56681ee4.png)

### 服务器端的并发实现
上文已经提到服务器接受连接请求后，accept是创建一个新的套接字描述符来与其连接的，然后自己仍然listen监听请求。

这样一来如果服务器端建立了很多连接，那么一个进程就需要处理很多个连接，显然不好，很多连接**来不及处理**，因此不如fork出子进程来处理每个连接，实现如下：

```cpp
while(1)
{
	newfd = accept(sfd, (void *)&raddr, &rlen); //接收连接并且记录发起连接的客户端地址
	if(newfd < 0)
	{

		perror("accept()");
		exit(1);
	}
	pid = fork();
	if(pid < 0)
	{
		perror("fork()");
		exit(1);
	}
	if(pid == 0) //子进程继承了父进程的文件描述符表，当然包括那个套接字newsfd
	{
		inet_ntop(AF_INET, &raddr.sin_addr, ipstr, IPSTRSIZE);
		printf("Clinet:%s:%d\n", ipstr, ntohs(raddr.sin_port));
		server_job(newfd);
		close(newfd);	
		exit(0); //处理完后子进程直接退出
	}
	else //父进程需要关闭本进程的文件描述符表的newfd，如果不关套接字无法正常关闭
	{
		close(newfd);
		wait(NULL);
		continue;
	}
}
```
* 子进程处理完后需要**及时退出**，否则可能fork指数增长
* 父进程需要把自己本进程的newfd**及时关闭**，因为父子进程newfd指向的是同一个file结构体，子进程处理完后close(newfd)**引用计数**不为0，套接字就无法及时关闭
* 如果这样的话，干脆在子进程那里直接`close(sfd);`也可以，毕竟它也用不到，避免不必要的错误吧还是。。

### client.c服务器端实现

这里我们不再拘泥于使用`recv`这种函数了，由于socket返回的本身就是一个文件描述符，我们完全可以**用文件操作来对套接字进行读写**。实现如下：
```cpp
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "proto.h"
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


int main(int argc, char **argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage...\n");
		exit(1);
	}
	int sfd;
	FILE *fp;
	long long stamp;
	struct sockaddr_in raddr;
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sfd < 0)
	{
		perror("socket()");
		exit(1);
	}
	//bind();绑定端口可省略，会自动分配一个端口
	raddr.sin_family = AF_INET; //填写服务器的IP和端口信息
	raddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, argv[1], &raddr.sin_addr); //从命令行获取想连接的服务器IP
	if(connect(sfd, (void*)&raddr, sizeof(raddr)) < 0) //尝试连接服务器
	{
		perror("connect()");
		exit(1);
	}

	//recv(); 这里不用recv，直接来个文件操作完全OK, sfd就是个文件描述符
	fp = fdopen(sfd, "r+");//用fdopen打开文件描述符获取FILE*
	if(fp == NULL)
	{
		perror("fdopen()");
		exit(1);
	}
	if(fscanf(fp, FMT_STAMP, &stamp) < 1)
	{
		fprintf(stderr, "Bad frmat!\n");
		exit(1);
	}
	else
	{
		fprintf(stdout, "RECEIVE STAMP: %lld\n", stamp);
	}
	fclose(fp);
	close(sfd);
	exit(0);
}
```

* 用fdopen打开套接字的文件描述符，返回一个FILE指针，后续操作就变成**标准IO**了，当然也可以直接用系统调用IO比如read来操作。
* 同样客户端不需要显式bind一个端口，系统会自动分配。只需要指定要connect的服务器的IP和端口即可，向服务器端发送请求
* 三次握手后connect成功结束，连接已经建立，就可以向套接字读写内容了。

./server先启动服务器，然后./client与服务器的TCP连接，得到如下结果:

服务器端知道对面的客户端的IP和端口号：

![image](https://user-images.githubusercontent.com/55400137/152625899-4acea03a-446d-4534-8a08-7b6f4ec00a27.png)

客户端也收到了服务器发来的时戳：

![image](https://user-images.githubusercontent.com/55400137/152625936-647dd169-1fa5-49e6-9792-87fe980d9d9b.png)

