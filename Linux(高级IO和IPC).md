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
涉及函数为mmap，将文件/设备的内容映射到进程空间的某块地址处
```cpp
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset); //映射到进程空间中addr开始长度为length的部分
```
## 文件锁

