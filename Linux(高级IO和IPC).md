# 高级IO
## 非阻塞IO与有限状态机编程
### 阻塞IO和非阻塞IO
* 首先需要明确，**读写常规文件没有阻塞的概念**，read一定会在有限时间内返回，因此在文件系统那部分没有提到到阻塞；但是由于Linux下万物皆文件，对于一些特殊的文件，比如**终端设备文件和网络**： 如果终端没上有**输入回车**，或者网络上**没有收到**数据包，那么调用read读就会出现**阻塞**，即若数据未就绪用户进程会sleep，后面将详细讲解。
* 注意是否阻塞取决于**文件的打开方式**，read/write被阻塞也是因为文件**默认以阻塞方式打开**导致的系统调用被阻塞。
 * 用open打开文件时可以指定用阻塞方式打开还是非阻塞方式：
 ```cpp
 //阻塞方式打开：
 int fd = open("/dev/tty", O_RDWR|O_NONBLOCK);
 //非阻塞方式打开：
 int fd = open("/dev/tty", O_RDWR);
 ```
 * 同样可以使用文件描述符管家函数来改变文件的**打开方式**：
 ```cpp
 fcntl(fd, F_SETFL, flags | O_NONBLOCK); //F_SETFL表示设置打开方式
 ```
 
* 阻塞IO：当发起系统调用IO后若内核缓冲区无数据，进程(线程)阻塞在IO操作上，并且**让出CPU**，知道数据就绪后**内核**将数据拷贝到用户线程，用户线程才解除阻塞状态。

* 非阻塞IO：当进程发起系统调用IO后，如果内核缓冲区没有数据，那么进程就返回一个**错误**而**不会阻塞**住，这个错误是`EAGAIN`，和阻塞IO中的`EINTR`一样是个假错；之后**不断询问内核**数据是否就绪，也就是说会**一直占用CPU**；知道内核数据就绪后，并且再次收到了线程的询问，那么**内核**将数据拷贝到用户线程后返回结束。

### 各种IO模型的区别
* 所有IO操作都可以分成**两个阶段**
  * 等待数据准备就绪
  * 将数据从内核态拷贝到用户空间

* 所有的IO模型都是根据其实是根据它在上述**两个阶段的不同表现**(用户进程或者线程是否阻塞)来区分的!!!:
  * 在第**一**个阶段用户进程是否阻塞用来**区分阻塞/非阻塞**
  * 在第**二**个阶段用户是否阻塞用来**区分同步/异步**

阻塞IO：用户进程在第一个阶段堵塞，第二个阶段也阻塞，两个阶段**都阻塞**

非阻塞IO：用户进程在第一个阶段不阻塞，第二个阶段阻塞

异步IO：异步IO实际上是用户进程发起read操作之后，就会立刻收到一个返回，所以用户进程就可以去完成其他的工作，而不需要阻塞；直到数据准备就绪并且完成了从内核空间向用户空间拷贝的工作，这时用户进程会收到一个通知，告诉他read操作已完成，用户进程在两个阶段**都不会阻塞**

可以看到除了异步IO都不阻塞，其余IO都至少在第二个阶段阻塞，都是同步IO：

![image](https://user-images.githubusercontent.com/55400137/150969766-9274ac38-9ac7-4e79-ab4e-d2eb7c359b0e.png)

【参考文章】：
[理解5种IO模型](https://cloud.tencent.com/developer/article/1684951)

### 有限状态机编程
有限状态机编程思想就是用来解决复杂流程的问题，是C语言中**抗衡**面向对象设计模式的利器，因为过程式代码很多情况下难以维护，但是fsm很好地解决了这个问题。

* 复杂流程：如果程序的自然流程是**结构化**的，即一步步**流水线**执行，无任何分支，那就是简单流程；否则就是**复杂流程**，比如有很多判断操作，不同的条件会执行不同的动作，跳转等等，这样的流程**不是结构化**的。
* 实例
源码见`relay.c`。
首先确定状态，copy操作的状态有四个：读、写、报错、终止，即：
```cpp
enum{
 STATE_R = 1,
	STATE_W,
	STATE_Ex,
	STATE_T
};
```
以及状态机的结构，最基本的里面有state状态，以及需要的各种**上下文**，在编写的时候如果某个**属性需要被下一个状态用到**，那么这个属性就需要加入状态机中：
```cpp
struct fsm_st
{
	int state;
	int sfd; //读写状态一直都要用到
	int dfd;
	int len; //在读状态得到的len显然需要在写状态中用到，因此添加该属性
	int pos; //读状态设置好后需要用到写状态中
	char *errstr; //读写状态设置好后需要用到出错状态
	char buf[BUFSIZE]; //读好后写状态要用到
};
```
找两个无需登录的终端`/dev/tty11`和`/dev/tty12`，通过`ctrl+alt+F11/F12`来切换。由于**终端设备**文件涉及**阻塞**问题，这时就需要考虑打开方式了，我们规定都以**非阻塞**方式打开，在realy函数中，都设置fd：因为不能确定用户都是用非阻塞方式打开，所以需要在relay中设置，并且在结束时**恢复原样**：
```cpp
fd1_save = fcntl(fd1, F_GETFL);
fcntl(fd1, F_SETFL, fd1_save|O_NONBLOCK);
fd2_save = fcntl(fd2, F_GETFL);
fcntl(fd2, F_SETFL, fd2_save|O_NONBLOCK);
....
fcntl(fd1, F_SETFL, fd1_save); //结束时恢复原样
fcntl(fd2, F_SETFL, fd2_save);
```
之后就是设置两个状态机的初始状态并且让状态机不断跑，可以看到初始状态都是`STATE_R`读状态：
```cpp
//设置两个状态机的初始状态
fsm12.state = STATE_R;
fsm12.sfd = fd1;
fsm12.dfd = fd2;
fsm21.state = STATE_R;
fsm21.sfd = fd2;
fsm21.dfd = fd1;
//让状态机不断跑
while(fsm12.state != STATE_T ||fsm21.state != STATE_T)
{
 fsm_driver(&fsm12);
 fsm_driver(&fsm21);
}
```

接下来推动状态机的函数`fsm_driver`就是对于不同状态case下的动作以及下一个状态的转换：
读状态即非阻塞read，读到buf里根据len判断是否成功，若成功则装换成写状态；失败则转换成报错状态
```cpp
case STATE_R:
	fsm->len = read(fsm->sfd, fsm->buf, BUFSIZE);
	if(fsm->len == 0) //出错
		fsm->state = STATE_T;
	else if(fsm->len < 0)
	{
		if(errno == EAGAIN)
			fsm->state = STATE_R; //假错继续读，也就是指向自身的箭头
		else
		{
			fsm->state = STATE_Ex; //真错设置errstr
			fsm->errstr = "read()";
		}
	}
	else
	{
		fsm->state = STATE_W; //读成功转到写
		fsm->pos = 0;
	}
	break;
```
写状态为了保证len长度全部写进去，需要一个pos属性，这个也是在编写过程中想到的
```cpp
case STATE_W:
	ret = write(fsm->dfd, fsm->buf + fsm->pos, fsm->len); //ret属性没有状态需要，因此不需要加入状态机
	if(ret < 0)
	{
		if(errno == EAGAIN) //假错
			fsm->state = STATE_W;
		else
		{
			fsm->state = STATE_Ex; //出错
			fsm->errstr = "write()";
		}
	}
	else
	{
		fsm->pos += ret;
		fsm->len -= ret;
		if(fsm->len == 0)
			fsm->state = STATE_R;
		else
			fsm->state = STATE_W; //指向自身的箭头本身就构成循环了

	}
	break;
```
剩下的出错态：
```cpp
case SATET_Ex:
	perror(fsm->errstr);
	fsm->state = STATE_T; //跳转到终止态
```
终止态啥的也没什么可写的动作，直接exit也行。

其中main函数是测试框架，我们实现的状态机主要在`relay`函数和`fsm_driver`函数。实现mycopy函数，由于打开文件的方式选用**非阻塞**：因此需要避免没读到数据后返回`EAGAIN`这个**假错**，因为这个错误不是read函数的问题，只是我尝试读一次数据没有读到而已。


## IO多路转接/复用
通过**一个**线程完成对**多个**文件描述符的监控。

```cpp
select();//非常古老，因此移植性比较好
poll();  // 对select的改进
epoll(); //效率高，但是Linux的方言(man手册在第七章)，不好移植
```
`fd_set`就是个文件描述符集合类型
![image](https://user-images.githubusercontent.com/55400137/151126857-590759f7-c965-4e81-a224-343cba6ab70c.png)

**多个**的进程/线程的IO可以**注册**到一个复用器（select）上，然后用**一个**进程调用该select， select会**监听所有**注册进来的IO；

如果select**所有**监听的IO在内核缓冲区**数据都没有就绪**，select调用进程会被阻塞；而当**任一**IO在内核缓冲区中有可数据时，select调用就会返回；

而后select调用进程可以自己或通知另外的进程（注册进程）来再次发起读取IO，读取内核中准备好的数据。

可以看到，**多个**进程注册IO后，**只有**另一个select**调用进程**被阻塞，**所谓复用**就体现在了这里。

* 区别：Linux中IO复用的实现方式主要有select、poll和epoll：
	* select：注册IO、阻塞扫描，监听的IO最大连接数不能多于FD_SIZE；因为是**数组**实现
	* poll：原理和Select相似，没有数量限制，但IO数量大扫描线性性能下降；**链表**实现
	* epoll ：事件驱动不阻塞，mmap实现内核与用户空间的消息传递，数量很大，Linux2.6后内核支持；**红黑树**实现

### select函数
```cpp
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
//nfds是监视描述符最大+1，后面三个fd_set，可读文件集、可写文件集、异常文件集，timeout是超时设置，NULL表示阻塞
void FD_CLR(int fd, fd_set *set); //从set中删除fd描述符
int  FD_ISSET(int fd, fd_set *set); //判断文件描述符是否在set中
void FD_SET(int fd, fd_set *set); //添加fd到set中
void FD_ZERO(fd_set *set); //清空set
```
select可以实现一个安全的休眠，即`select(0, NULL, NULL, NULL, time);` time就是睡眠时间。
#### 缺陷
* select是**用事件为单位组织文件描述符的**，可以监控的事件只有读、写、异常三种（对应三个fd_set），可以监控的事件较少
* 更重要的缺陷：select的**监控现场和监控结果共用同一块内存**，有文件就绪后将**删除所有**fd_set中未就绪的文件描述符，因此我们必须continue重新设置fd_set，开销很大
* 优点就是移植性好，因为古老

#### 实例
在之前写的两个终端复制的代码`relay.c`中：
```cpp
while(fsm12.state != STATE_T || fsm21.state != STATE_T)
{
	fsm_driver(&fsm12);
	fsm_driver(&fsm21);
}
```
如果状态不是终止态的话就会**忙等**，CPU的占用率非常高，因此使用select监控：

```cpp
fd_set rset, wset;
while(fsm12.state != STATE_T || fsm21.state != STATE_T)
{
	//布置监控任务
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	if(fsm12.state == STATE_R) //如果状态机fsm12为读状态
		FD_SET(fsm12.sfd, &rset); //fsms12.sfd就是fd1
	if(fsm12.state == STATE_W) //如果状态机fsm12为写状态
		FD_SET(fsm12.sfd, &wset);
	if(fsm21.state == STATE_R) //如果状态机fsm21为读状态
		FD_SET(fsm21.sfd, &rset); 
	if(fsm21.state == STATE_W) //如果状态机fsm21为写状态
		FD_SET(fsm21.sfd, &wset);
	
	//用select监控
	if(select(max(fd1, fd2)+1, &rset, &wset, NULL, NULL) < 0)
	{
		if(errno == EINTR) //如果假错必须重新设置fd_set，因为select会清空监控结果
			continue;
		perror("select()");
		exit(1);
	}
	//查看监控结果
	if(FD_ISSET(fd1, &rset) || FD_ISSET(fd2, &wset)) //1可读2可写
		fsm_driver(&fsm12);
	if(FD_ISSET(fd2, &rset) || FD_ISSET(fd1, &wset)) //2可读1可写
		fsm_driver(&fsm21);
}
```
这样程序会**阻塞到**select函数处，进程阻塞就是sleep了，因此CPU的占用率几乎无变化，而不像之前忙等那样几乎占满。
* 还有个问题就是上面的代码只推动状态机处于读态或者写态，而STATE_Ex态到STATE_T态的推动是**无条件**的，因此我们做一条**线**：
```cpp
enum
{
	STATE_R = 1,
	STATE_W,
	STATE_AUTO, //这个状态其实就是个占位符
	STATE_Ex,
	STATE_T
};
```
因此在select监控之前，需要首先判断是否可以**无条件推动**，即`fsm.state > STATE_AUTO`，因此添加代码：
```cpp
if(fsm12.state > STATE_AUTO)
	fsm_driver(&fsm12);
if(fsm21.state > STATE_AUTO)
	fsm_driver(&fsm21);
//之后才是select监视
```

### poll函数
poll函数是对select函数的改进，最明显的改进就是将**感兴趣**的事件和**已经发生**的事件**分开存储**，而不是共用内存。

poll是**以文件描述符为单位组织事件**，引入pollfd结构体来实现。并且poll可以监控的事件更多，不像select只能监控三种事件。

```cpp
int poll(struct pollfd *fds, nfds_t nfds, int timeout); //前两个参数是内存打包：数组首地址和元素个数；timeout超时设置为-1表示阻塞
struct pollfd {
       int   fd;         /* file descriptor */
       short events;     /* requested events */ //感兴趣的事件，用位图表示，就是一堆宏或
       short revents;    /* returned events */  //在这个文件描述符上发生的事件
   };
```
返回值为revents非0的结构体的**个数**，即**已经就绪的**文件描述符的个数。失败的话返回-1并设置errno，会有假错

#### 实例
同样三步走：布置监控任务；监控；查看监控结果
```cpp
struct poll_fd pfd[2]; //自定义结构体数组
pfd[0].fd = fd1;
pfd[1].fd = fd2;
while(fsm12.state != STATE_T || fsm21.state != STATE_T)
{
	//布置监控任务
	pdf[0].events = 0;	
	if(fsm12.state == STATE_R)  //当fsm12可读
		pdf[0].events |= POLLIN;	
	if(fsm21.state == STATE_W)  //当fsm21可写
		pdf[0].events |= POLLOUT;
	if(fsm21.state == STATE_R)  //当fsm21可读
		pdf[1].events |= POLLIN;
	if(fsm12.state == STATE_W)  //当fsm12可写
		pdf[1].events |= POLLOUT;
		
	//用poll监控
	while(poll(pfd, 2, -1) < 0)
	{
		if(errno == EINTR) //如果假错不用重新布置监控任务，因为poll将发生事件和监控现场分开存储
			continue;
		perror("select()");
		exit(1);
	}
	//查看监控结果，即revents项
	if(pdf[0].revents &POLLIN || pdf[1].revents &POLLOUT) //1可读2可写
		fsm_driver(&fsm12);
	if(pdf[1].revents &POLLIN || pdf[0].revents &POLLOUT) //2可读1可写
		fsm_driver(&fsm21);
}
```
可以看到这样程序执行后，将会阻塞到select函数处，CPU的占用率没有明显波动而不像之前那样几乎占满。

### epoll函数
epoll是Linux的一种机制，涉及到三个**系统调用**。定义了一套完整的接口让用户使用，内部的实现**全部在内核态**，而不像select和poll那样需要在用户态建立自己的结构体（比如pollfd），创建实例由epoll_create()系统调用实现。
```cpp
int epoll_create(int size); //创建epoll实例并返回文件描述符epfd
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event); //在epfd示例中进行op操作
EPOLL_CTL_ADD //op操作，添加fd
EPOLL_CTL_MOD //修改对于fd的关心事件
EPOLL_CTL_DEL //删除fd

struct epoll_event {
       uint32_t     events;    /* Epoll events */ //事件也是一个位图，和poll中的那样一堆宏
       epoll_data_t data;      /* User data variable */ //这是一个共用体，里面包含fd项
   };

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout); //等待epfd所关心的事件，并将发生的事件放到epoll_event类型的数组里（内存打包
// timeout为-1表示阻塞
```
#### 实例
```cpp
int epfd = epoll_create(10); //返回文件描述符epfd
struct epoll_event ev;
ev.events = 0; //暂时不添加事件
epoll_ctl(epfd, EPOLL_CTL_ADD, fd1, &ev); //添加fd1
ev.events = 0;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd2, &ev); //添加fd2
if(epfd < 0)
{
	perror("epoll_create()");
	exit(1);
}
while(fsm12.state != STATE_T || fsm21.state != STATE_T)
{
	//布置监控任务
	ev.data.fd = fd1;
	ev.events = 0; //修改fd1的事件
	if(fsm12.state == STATE_R)  //当fsm12可读
		ev.events |= EPOLLIN; 
	if(fsm21.state == STATE_W)  //当fsm21可写
		ev.events |= EPOLLOUT; 
	epoll_ctl(epfd, EPOLL_CTL_MOD, fd1, &ev);
	
	ev.events = 0; //修改fd2的事件
	if(fsm21.state == STATE_R)  //当fsm21可读
		ev.events |= EPOLLIN; 
	if(fsm12.state == STATE_W)  //当fsm12可写
		ev.events |= EPOLLOUT;
	epoll_ctl(epfd, EPOLL_CTL_MOD, fd2, &ev);
		
	//用epoll_wait监控
	while(epoll_wait(epfd, &ev, 1, -1) < 0) //数组大小为1
	{
		if(errno == EINTR) 
			continue;
		perror("epoll_wait()");
		exit(1);
	}
	//查看监控结果
	if(ev.date.fd==fd1 && ev.events&EPOLLIN || ev.date.fd==fd2 && ev.events&EPOLLOUT) //1可读2可写
		fsm_driver(&fsm12);
	if(ev.date.fd==fd2 && ev.events&EPOLLIN || ev.date.fd==fd1 && ev.events&EPOLLOUT) //2可读1可写
		fsm_driver(&fsm21);
}
close(epfd);
```

【参考文章】:
[IO多路复用讲解](https://juejin.cn/post/7051170770491441182)
[理解IO多路复用的实现](https://juejin.cn/post/6882984260672847879)
[select.poll.epoll的区别](https://juejin.cn/post/6850037276085321736)
## 其他读写函数
主要是两个函数，实现对多个buffer的读写
```cpp
ssize_t readv(int fd, const struct iovec *iov, int iovcnt); //iovec就是个小的buffer，iov和iovcnt又是个内存打包，也就是很多个小buffer
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
struct iovec {
       void  *iov_base;    /* Starting address */
       size_t iov_len;     /* Number of bytes to transfer */
   };

```
## 存储映射IO
涉及函数为mmap，将文件/设备的内容映射到进程空间的某块地址处。

这样一来，原本在磁盘上的内容，想要去读写就需要open打开再去read或者write，但是一旦映射到了内存上，就可以**使用指针**去操作，这样对文件的操作就有了更多的方法，当然也就可以修改内存直接改变磁盘文件。
【参考文章】：
[mmap是什么 为什么 怎么用](https://www.jianshu.com/p/57fc5833387a)

[浅谈Linux内存完整的管理机制](https://github.com/0voice/kernel_memory_management/blob/main/%E2%9C%8D%20%E6%96%87%E7%AB%A0/%E6%B5%85%E8%B0%88Linux%E5%86%85%E5%AD%98%E7%AE%A1%E7%90%86%E6%9C%BA%E5%88%B6.md)

[深入浅出Linux内存管理](https://github.com/0voice/kernel_memory_management/blob/main/%E2%9C%8D%20%E6%96%87%E7%AB%A0/%E6%B7%B1%E5%85%A5%E6%B5%85%E5%87%BAlinux%E5%86%85%E5%AD%98%E7%AE%A1%E7%90%86%EF%BC%88%E4%B8%80%EF%BC%89.md)
```cpp
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset); //映射到进程空间中addr开始长度为length的部分，成功返回起始位置
int munmap(void* addr, size)t length); //解除映射
```
那么这就有个问题，我们不知道哪块内存是安全的，因此可以填NULL让mmap自己找一块安全的内存。
![image](https://user-images.githubusercontent.com/55400137/151473846-eebd5ce0-8cdf-41ca-9469-9fbbb77fd4d8.png)

port表示权限，读写等等

flags标记，也是个位图，确定特殊要求，必选的有`MAP_SHARED`或`MAP_PRIVATE`；后面有可选项，比较重要的有**匿名映射**选项`MAP_ANONYMOUS`

fd就是打开的文件

offset表示从fd文件offset偏移量开始映射length个字节到addr起始处
### 实例
将文件映射到进程的内存空间，并且数文件中字符a的个数：
```cpp
int main(int argc, char **argv)
{
	struct stat st;
	char *str;
	if(argc < 2)
	{
		fprintf(stderr, "Usage:too fewer args...\n");
		exit(1);	
	}
	int fd = open(argv[1], O_RDONLY);
	if(fd < 0)
	{
		perror("open()");
		exit(1);
	}
	if(fstat(fd, &st)<0) //st是为了获取文件长度
	{
		perror("fstat()");
		exit(1);
	}
	str = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0); //将文件fd映射到进程的内存空间，返回映射好的首地址str
	if(str == MAP_FAILED)
	{
		perror("mmap()");
		exit(1);
	}
	close(fd);      //！映射完之后原文件就可以关闭了
	int i, count;
	for(i = 0; i<st.st_size;i++)
	{
		if(str[i] == 'a')
			count++;
	}
	printf("a's count is %d\n", count);
	munmap(str, st.st_size); //解除映射
}
```

### 父子进程mmap实现通信
mmap可以当作malloc那样使用**动态分配内存**，即不依赖于任何文件，fd参数为-1，只能选择**匿名映射**MAP_ANONYMOUS。即：
```cpp
mmap(NULL, MEMSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
```
匿名映射会将一个**内核创建**的全为进制零的**匿名文件文件**映射到虚拟内存中。由于子进程会duplicate父进程的内存空间，因此它们会映射到**同一个**匿名文件从而实现通信。

代码如下：子进程写匿名文件，父进程读，实现通信。
```cpp
int main(int argc, char **argv)
{
	char *ptr;
	ptr = mmap(NULL, MEMSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	
	if(ptr == MAP_FAILED)
	{
		perror("mmap()");
		exit(1);
	}
	pid_t pid;
	pid = fork();
	if(pid < 0) //failure
	{
		perror("fork()");
		munmap(ptr, MEMSIZE);
		exit(1);
	}
	if(pid == 0) // child
	{
		strcpy(ptr, "hello!");
		munmap(ptr, MEMSIZE);
		exit(0);
	}
	else //  parent
	{
		wait(NULL);
		puts(ptr);
		munmap(ptr, MEMSIZE);
		exit(0);
	}
}
```

## 文件锁
主要是为了解决多进程操作同一个文件的竞争问题，可以锁自己工作文件的一**部分**。
涉及到几个函数
```cpp
fcntl();
flock();

```
之前我们通过**多线程并发**，`pthread_create`和`pthread_mutex`互斥量来实现。这里是**多进程并发**
```cpp
//下面就是文件锁锁住临界区
fd = fileno(fp);
lockf(fd, F_LOCK, 0);
fgets(linebuf, BUFSIZE, fp);
lokc(fd, F_ULOCK, 0);
```

## 手写线程间的管道
我们经常使用进程间的管道，比如`ls | more`，管道把一个进程的**标准输出**作为另一个进程的**标准输入**，是内核提供的机制来实现**进程**间通信。

如果使用内核提供的管道机制，让两个线程通信就有点杀鸡用牛刀的感觉了，两个线程本身就共享同一个进程的资源，犯不着用内核提供的管道。

这里用线程互斥量和条件变量实现一个简单的管道，其实就是**读者写者**在一块**循环队列**上读写数据。这个队列可以看作线程间的管道，使用时需要先注册读者/写。

不好的一点就是没有实现**权限封装**，比如写者可以随意调用read函数/读者同样可以调用write函数，可以像Unix文件系统那样再次进行封装，在系统打开表中增加**权限项**，每次读写操作时先判断权限是否满足。

实现见`mypipe.md`

# 进程间通信(IPC)
## 同一台主机上的进程
### 管道
IPC的管道机制是内核提供的功能，管道文件**均由内核创建**，**单工**通信，并且有**自同步机制**，区分命名管道和匿名管道：
> **单工**通信模式：数据传输为**单向**，固定一端为读端一端为写端。
* 管道文件和普通文件的区别在于：
	* 管道有同步机制：缓冲区空时读者阻塞，缓冲区满写者阻塞
	* 必须同时有读者和写者才能使用管道。
	* 和我们之前实现的一样：内核提供的管道也是个**环形队列**
#### 匿名管道

只能用于**父子进程**之间的通信。
> 临时文件：
> ```cpp
> FILE * tmpfile(void); // 返回一个FILE流指针，而不用给它名字
> ```
> 以二进制更新模式(wb+)创建**临时文件**。被创建的临时文件会在流关闭的时候或者在程序终止的时候**自动删除**，也就是说我们不关心这个文件叫什么名字，返回一个FILE指针，我们只管用指针操纵
> 这个文件。

由于没有办法在磁盘上看到匿名管道文件（没有名字），所以如果进程之间**没有亲缘关系**那么就**无法使用同一个**匿名管道，而如果fork的话子进程会duplicate父进程的打开文件，因此可以使用同一个。

整个流程如下：

![image](https://user-images.githubusercontent.com/55400137/151652394-2ddb33bd-f3ac-4512-afc2-559c01434d64.png)

匿名管道创建函数：
```cpp
int pipe(int pipefd[2]);
/*    The array pipefd is used to return two
 *    file  descriptors  referring to the ends of the pipe.  pipefd[0] refers
 *    to the read end of the pipe.  pipefd[1] refers to the write end of  the
 *    pipe. 
 *    写端写入的数据将**被内核缓存**直到读端开始读 */
```
pipefd就是个输入指针，函数内部将修改数组元素为**文件描述符**使得`pipefd[0]`为**读端**，`pipefd[1]`为**写端**。

父进程pipe之后获得两个fd，再fork，这时我们根据读写需要在父子进程内关闭不用的fd（因为fork后父子进程都有读写两个fd），当然不关也可以但是可能有出错隐患。如下：
```cpp
#define BUFSIZE 1024
int main()
{
	int pipefd[2];
	char buf[BUFSIZE];
	pid_t pid;
	if(pipe(pipefd) < 0)
	{
		perror("pipe()");
		exit(1);
	}
	pid = fork();
	if(pid < 0)
	{
		perror("fork()");
		exit(1);
	}
	if(pid == 0) //child read
	{
		close(pipefd[1]); //0为读1为写
		int len = read(pipefd[0], buf, BUFSIZE);
		write(1, buf, len); //写到标准输出
		close(pipefd[0]);
		exit(0); //子进程退出
	}
	else // parent write
	{
		close(pipefd[0]);
		write(pipefd[1], "hello!", 6); //写到标准输出
		close(pipefd[1]);
		wait(NULL); //收尸子进程，就是子进程那点pid号退出状态和运行时间等等少量信息（内存和资源在退出时全部释放了也就是“死了”
		exit(0); 
	}
}
```
更贴近实际的模型：S/C服务器，C端父进程用socket获取S发来的数据后fork子进程，父子进程通过**匿名管道**通信，子进程exec变成mpg321解码器来解析管道内的数据包：
```cpp
if(pid == 0) //child read
{
	close(pipefd[1]); //0为读1为写
	dup2(pipefd[0], 0); //输入重定向到管道的读端
	close(pipefd[0]);
	int fd = open("/dev/null", O_RDWR);
	dup2(fd, 1);
	dup2(df, 2);
	execl("/usr/bin/mpg321", "mpg321", "-", NULL); //-表示从标准输入读数据，之前已经重定向过了
	perror("execl()"); //能执行到这步说明exec一定错了，否则内存映像早变了
	exit(1);
}
```
> 对于`mpg321`的使用：`cat jay01.mp3 | mpg321 -`其中-表示从标准输入获取文件。

#### 管道命令
比如`ls | grep `连接两个命令：即将一个进程的输出作为另一个进程的输入，这里用到的就是**匿名管道**，当命令里面包含一个**管道符**`|`时，就会先pipe再fork，当然涉及到shell进程的细节：

对于管道命令`cmd1 | cmd2`：
1) shell进程首先fork出子进程再exec变成cmd1，

2) 然后cmd1先pipe后fork+exec得到cmd2，这时cmd1和cmd2已经被管道连接了，

3) 之后父子进程用dup2命令将标准输入输出重定向**连到管道上**，

4) 之后就是关闭不需要的fd即可。

大致像下面这样：

![image](https://user-images.githubusercontent.com/55400137/151652526-9204aafc-f3d8-4797-bafa-c12f28b62dbc.png)


#### 命名管道

和匿名管道的区别在于FIFO有文件名可以供**没有**亲缘关系的进程通信。

在Linux的man手册中，**命名管道就叫`FIFO`**，匿名管道才叫`pipe`。因此管道其实就是个**队列**，我们之前线程管道实际上也是个队列，说明管道和队列的行为是一致的。
之前文件系统讲过文件类型有：`dcb-lsp`，命名管道就是磁盘上一个文件类型为`[p]`的**管道文件**，因为是文件所以使用方式平平无奇：用`open`

相关函数：
```cpp
int mkfifo(const char *pathname, mode_t mode); //创建FIFO的特殊文件-->命名管道(a named pipe)
```
和open的参数很像，第一个pathname为文件名，第二个为权限(mode & ~umask)。

也有mkfifo**命令**是通过mkfifo函数封装的：`mkfifo mypipe`，得到文件如下：
```cpp
prw-r--r-- 1 njucs njucs        0 1月  29 14:44 mypipe
```
可以看到文件类型为`p`，并且是个空文件。

**命名管道的使用**：同样必须**同时有**读者和写者，否则就会**阻塞**；并且有自同步机制，比如：
```cpp
date > mypipe
```
会发现**阻塞了**（没有输入提示）因为只有写者没有读者，需要另起一个终端键入：
```cpp
cat mypipe
```
产生一个读者，管道才会开始运行，此终端输出date内容，写终端也正常结束了。

【参考文章】: [Linux中管道的实现](https://segmentfault.com/a/1190000009528245)  [shell中管道命令的实现](https://juejin.cn/post/6844903695717498887#comment)

#### 双工管道
由于传统管道信息流动方向是固定的，我们想要实现双向的信息流动：那么就创建两个管道，一个父读子写，一个父写子读就可以实现了。

### XSI -> SysV
通过`ipcs`可以看到XSI -> SysV提供的几种进程间通信机制：
```cpp
--------- Message Queues -----------
key       msqid      拥有者  权限     已用字节数 消息      

------------ Shared Memory --------------
key       shmid      拥有者  权限     字节     连接数  状态      
0x00000000 131072     njucs      777        16384      1          目标       
0x00000000 327681     njucs      600        67108864   2          目标         

--------- Semaphore Arrays -----------
key        semid      拥有者  权限     nsems     

```
其中：

Message Queues： 消息队列  msg
Semaphore Arrays： 信号量数组  sem
Shared Memory: 共享内存 shm
key: 使用key来确定通信的双方拿到的是**同一种**通信机制。

这部分涉及到的函数命名规则为：`xxxget(获取); xxxop(操作); xxxctl(控制)`
> 用man手册查看时，就可以按照这种方式查询，比如获取一个消息队列 msgget, 获取信号量数组 semget， 获取一块共享内存 shmget

注意区分:

主动端：**先发包**的一方
被动端：**先收包**的一方

> 比如登录QQ我就是主动端，因为是我先发包请求QQ服务器；但是被动端先运行，即QQ服务器先运行等待我的请求包。


#### 消息队列msg
和管道不一样，消息队列是**双工**的，通信双方都能读写。
相关函数：
```cpp
int msgget(key_t key, int msgflg); //创建和访问一个消息队列，返回一个消息队列标识符msgid
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg); //把消息添加到消息队列，msgid是由msgget函数返回的消息队列标识符
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg); //接收
int msgctl(int msqid, int cmd, struct msqid_ds *buf); //控制消息队列，比如IPC_RMID就是删除
```
* 参数相关
	* 和其他两个IPC机制一样，程序必须提供一个**键key**来命名某个**特定的**消息队列；msgflg是一个**权限标志**，表示消息队列的访问权限，它与**文件的访问权限**一样。
	* msgflg可以与`IPC_CREAT`做或操作，表示当**key所命名**的消息队列**不存在时创建**一个消息队列，如果key所命名的消息队列存在时，IPC_CREAT标志会被**忽略**，而只返回一个标识符。
	* msg_ptr是一个指向准备发送消息的指针，但是消息的数据结构却有一定的要求，指针msg_ptr所指向的消息结构一定要是以一个`long mtype`成员变量开始的结构体，接收函数将用这个成员来确定**消息的类型**。所以消息结构要定义成这样：
```cpp
struct my_message{
    long int message_type;
    /* The data you wish to transfer*/
};
```
	* msg_sz是msg_ptr指向的**消息的长度**，注意是消息的长度，而**不是整个结构体的长度**，也就是说msg_sz是不包括`long mtype`的长度。
	* msgtype可以实现一种简单的**接收优先级**。如果msgtype为0，就获取队列中的第一个消息。如果它的值大于零，将获取具有相同消息类型的第一个信息。如果它小于零，就获取类型等于或小于msgtype的绝对值的第一个消息。
	

* 实例
我们使用IPC实际上就是**自己封装协议**，我们定义消息类型为：
```cpp
struct msg_st
{
        long mtype; //协议要遵循msgbuf约定
        char name[30];
        int math;
        int chinese;
};
```
让rcver接收者创建和销毁消息队列，snder获取创建好的消息队列。完整代码见：`msg.md`。

【参考文章】：[Linux进程间通信——消息队列](https://blog.csdn.net/ljianhui/article/details/10287879)
#### 信号量数组sem

#### 共享内存shm

## 不同主机上的进程
### 网络套接字socket
* 封装协议
协议就是通信双方约定的格式
