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
其中main函数是测试框架，我们实现的状态机主要在`relay`函数和`fsm_driver`函数。实现mycopy函数，由于打开文件的方式选用**非阻塞**：因此需要避免没读到数据后返回`EAGAIN`这个**假错**，因为这个错误不是read函数的问题，只是我尝试读一次数据没有读到而已。



## IO多路转接

## 其他读写函数

## 存储映射IO

## 文件锁

