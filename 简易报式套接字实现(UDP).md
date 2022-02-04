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
* 因为被动端首先运行，在bind套接字绑定时**必须指定监听端口**，**不能省略！**，否则主动端根本不知道要发给谁
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

## 广播的实现
**广播和多播仅用于UDP报式套接字，流式套接字无法实现多点通信**！！！
* 广播套接字的设置
我们看发送方sender，广播地址是255.255.255.255人尽皆知，重点在于套接字还需要设置一些内容才能真正**广播**，在`man 7 socket`的`Socket options`栏目中有：
```cpp
SO_BROADCAST
//    Set or get the broadcast flag.  When enabled,  datagram  sockets
//    are allowed to send packets to a broadcast address.  This option
//    has no effect on stream-oriented sockets.
```

从描述可以看到：**只有报式套接字UDP**才有广播选项，流式套接字TCP不能，因为流式套接字是**面向连接的**，**一对一确定**的，从手册也可以看到：`SO_BROADCAST`选项对于流式套接字**无效**。

设置套接字选项可以使用函数：
```cpp
// ...with the socket level set to SOL_SOCKET for all sockets.
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);    //获取套接字选项
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);  //设置套接字选项，设置套接字level层面的optname选项
```

```cpp
sfd = socket(AF_INET, SOCK_DGRAM, 0);
if(sfd < 0)
{
	perror("socket()");
	exit(1);
}
//设置广播socket选项
int val;
if(setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) < 0) //设置好广播选项
{
	perror("setsockopt()");
	exit(1);
}

strcpy(sbuf.name, "Zhang Huan");
sbuf.math = htonl(rand() % 100);
sbuf.chinese = htonl(rand() % 100);

raddr.sin_family = AF_INET;
raddr.sin_port = htons(RCVPORT); //设置对面信息
inet_pton(AF_INET,"255.255.255.255", &raddr.sin_addr); //全1(255.255.255.255)为广播地址

//sendto用于报式传输 send用于流式传输
if(sendto(sfd, &sbuf, sizeof(sbuf), 0, (void *)&raddr, sizeof(raddr)) < 0)
{
	perror("sendto()");
	exit(1);
}
```

同样接收方也设置socket：
```cpp
//设置socket选项
int val;
if(setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) < 0) //设置广播套接字，不像多播那样需要填写多播组IP信息
{
	perror("setsockopt()");
	exit(1);
}
```

如果send方发送正常而recv收不到内容，很可能是**防火墙**的原因，关闭防火墙就好了。

## 多播/组播的实现
多播地址属于D类地址，不分网络地址和主机地址，它的第1个字节的前四位固定为1110。 D类IP地址的范围为224.0.0.0～239. 255. 255. 255。

实现多播时：**发送方创建多播组**并且发送消息；**接收方加入多播组**并接收消息。

### 封装协议
在原有基础上增加**多播组号**的定义
```cpp
#define MGROUP "224.2.2.2"
```
有一个特殊的多播地址：`224.0.0.1`表示所有支持多播的设备都在这个组中，并且**无法离开**，就相当于255.255.255.255，从这里也可以看到**广播是多播的特例**。

### 发送方
* 多播套接字的设置

发送方需要**创建多播组**
广播发送需要在SOL_SOCKET层设置广播SO_BROADCAST选项，多播需要在IP层IPPROTO_IP设置多播选项：

`man 7 ip`的`Socket options`栏目中有：
```cpp
// The socket option level for IP  is  IPPROTO_IP.
IP_MULTICAST_IF    //创建多播套接字
IP_ADD_MEMBERSHIP  //加入多播组
IP_DROP_MEMBERSHIP //离开多播组
//相关结构体
struct ip_mreqn {
	struct in_addr imr_multiaddr; /* 多播组IP地址 */
	struct in_addr imr_address;   /* IP address of local interface */
	int            imr_ifindex;   /* interface index 网络索引号*/
};
```
**相关命令/函数**：
```cpp
ip ad sh  //查看网络索引号
if_nametoindex(); //通过网络设备命查看其网络索引号
ifconfig
```

需要修改的代码如下：
```cpp
raddr.sin_family = AF_INET;
raddr.sin_port = htons(RCVPORT); //设置对面信息为多播组
inet_pton(AF_INET, MGROUP, &raddr.sin_addr); //多播发送到多播组MGROUP

struct ip_mreqn mreq;
inet_pton(AF_INET, MGROUP, &mreq.imr_multiaddr); //设置要创建的多播组IP地址
inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address); //本机地址
mreq.imr_ifindex = if_nametoindex("eth0"); //网络索引号

if(setsockopt(sfd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0) //按照上面填写的内容创建多播套接字生效
{
	perror("setsockopt()");
	exit(1);
}
```

### 接收方
需要**加入多播组**用到`IP_ADD_MEMBERSHIP`选项，
修改代码如下：
```cpp
struct ip_mreqn mreq;
inet_pton(AF_INET, MGROUP, &mreq.imr_multiaddr); //要加入的多播组IP地址
inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address); //本机地址
mreq.imr_ifindex = if_nametoindex("eth0"); //网络索引号

if(setsockopt(sfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) //加入多播组
{
	perror("setsockopt()");
	exit(1);
}
```
