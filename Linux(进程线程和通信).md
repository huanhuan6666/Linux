> Linux系统C语言编程，继续胡言乱语中。。。
## 须知
### main函数
* C程序规定：main函数是程序的**入口**，也是程序的**出口**
* 一般main函数就是两个参数`int main(int argc, char **argv)`，也可以放第三个参数`char **env`环境变量，但是Linux对环境变量做了单独的处理，一般不怎么使用这个参数了
### 进程的终止
5种正常终止，3种异常情况终止必须牢记!!!

### 进程的正常终止(5种)：
  
#### ①从main函数return：
* 也就是`return 0`了
  
#### ②库函数`exit()`:
* 比如常用的`exit(0); exit(1);`查看man手册中对于exit()库函数的描述：

	```cpp
	void exit(int status);
	//The  exit() function causes normal process termination and 
	//the value of status & 0377 is returned to the parent (see wait(2)).
	```
	* 可以看到status是个int，显然这个范围非常大，因此返回地时候也是用了个mask掩码一下，**只取低8位**。
	* return的值和exit的返回值是给谁看？答案是**父进程**，谁创建了main这个进程谁就是父进程，比如我们在shell下面`./main`，那么**在这个场景下**，shell进程就是父进程。在shell里可以通过` echo $?`命令查看上一条命令的`exit status`即退出值。
	* 一般我们**约定**返回值为0，即我们常常写`return 0; exit(0);`作为程序的结束，如果不写的话正常结束的话也返回0.但这只是个约定而已。我们当然可以写成`return 6; exit(3);` 这样`echo $?`查看出来的就是`6 3`。

* 钩子函数atxit()
标准库函数，通过函数指针在进程内注册函数，进程在`exit()`或者`return`时会按照和注册顺序相反的顺序调用那些注册的函数，描述如下：
其实这就是**钩子**的意思：用atexit注册函数就好像挂钩子，钩子一个个挂，要取的时候只能从下往上一个个取，因此和挂(注册)的顺序相反
```cpp
int atexit(void (*function)(void)); // 这个function就是个钩子，一个个挂上去，没有参数没有返回值
//The atexit() function registers the given function to be called at normal process termination, 
//either via exit(3) or via return from the program's main().  Functions so registered are called 
//in the reverse order of their registration; no arguments are passed.
```
一般钩子函数都于**释放资源**，因为进程`exit`或者`return`时`“at” exit`会挨个调用所有的钩子，有点像**C++的析构函数**。

#### ③系统调用函数`_exit()`或者`_Exit()`:
这两个是系统调用函数，也就是说之前的`exit()`函数是封装了`_exit()`或者`_Exit()`，如下图：
![image](https://user-images.githubusercontent.com/55400137/150049493-87f04316-29f0-4dea-8263-6f93756a3594.png)

上图展示了`_exit()`或`_Exit()`系统调用的过程，可以看到是什么都不做，**直接扎到内核结束**，**不调用钩子函数**；而库函数`exit()`则需要**调用一系列终止处理程序**，包括注册的**钩子函数**，并且**清理标准IO**，然后才调用`_exit()`系统调用扎入内核结束。

* 因此如果程序中可能出现隐藏的错误，结束时调用`_exit()`函数直接扎入内核比较好，而不是`exit()`会调用钩子函数并刷新IO，因为这样很可能**让错误扩散**。
#### ④最后一个线程从其启动例程返回:
  
#### ⑤最后一个线程调用`pthread_exit()`函数:
  
### 进程的异常终止(3种)：
#### 调用库函数`abort()`
#### 接到一个信号并终止
#### 最后一个线程对其取消请求作出响应
	
### 命令行参数分析
常用于命令行参数分析的**库函数**：
`getopt()`;原型如下：
```cpp
int getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg; 
extern int optind, opterr, optopt;
```
其中`argc`和`argv`就是给main函数传的参数。`argv`中以`-`开头的字母就是可能的**选项**(option)，比如`ls -a -i`中的a和i。`optstring`是指定待匹配的选项列表。
getopt()每次只返回**一个字母**，就是成功匹配的选项，因此一般需要`while`循环调用。
getopt支持解析选项参数，比如`-y 12`，12是y的参数；也支持不定选项，那几个变量`optind`就是当前选项在`argv`里的下标。
`getopt_long()`支持解析长格式，具体看手册。
#### 练习
实现命令mytime，支持解析命令行参数`y M d H m S`，并且选项y和H可以带参数，默认将时间输出到stdout；也支持**可选**参数文件名，将时间输出到文件中。
之后补（

### 环境变量
* 就是一条条的键值对`KEY = VALUE`.
下面是几个重要的环境变量
```cpp
HOME	用户的主目录（也称家目录）
SHELL 	用户使用的 Shell 解释器名称
PATH	定义命令行解释器搜索用户执行命令的路径
EDITOR	用户默认的文本解释器
RANDOM	生成一个随机数字
LANG	系统语言、语系名称
HISTSIZE	输出的历史命令记录条数
HISTFILESIZE	保存的历史命令记录条数
PS1	Bash解释器的提示符
MAIL	邮件保存路径
```
其中PATH就是执行的各种命令的路径，比如`ls, cat, vim`等等，这些命令一般都存在一堆用冒号分隔的`/bin`二进制(binary)文件夹中，比如：
`PATH=/home/njucs/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin
`
* 执行每个程序时，都可以通过main函数把**当前进程的环境变量**传给它`char **env`，因此每个**进程的进程空间存有本进程的环境变量**
* 可以通过命令`export`或者`env`查看所有的环境变量。当然也可以在程序中`extern char **environ;`使用，通过C语言的学习我们直到这就是个**自停止**的指针而已。
* Linux支持给**不同用户**提供自定义的环境变量，因此，一个相同的环境变量会因为**用户身份的不同**而具有不同的值。
* 一些库函数也可以获取环境变量：`char *getenv(const char *name)`，就是传入key返回value；改变或增加环境变量`int setenv(const char *name, const char *value, int overwrite);`同样key和value。 

### C程序的存储空间布局
这里讲的都是虚拟内存，虚拟地址到物理地址的翻译就完全是ICS的内容了，而我们这里讨论的是OS层面的，**OS看到的就是虚拟内存**。
每个进程，如果32位(4G)的话那么内存空间：分成用户区和内核区，用户区低3G(0xc0000000)，内核区高1G。
* **用户区**就是典中典的**内存四区**，当然全局区还能细分以及动态库区这部分就不深究了。而实际上用户区不是从0开始的，32位机是**从0x08048000开始**的。
	* 从代码段开始向上，依次是已初始化数据段和未初始化数阶段，然后堆区栈区双向奔赴，中间夹个**共享库**。
	* 栈的上面就是重量级的**main函数参数和环境变量**了，这部分是在**用户区的最上方**。从这里就清楚了之前`setenv()`和`getenv()`函数的真面目了，环境变量本身就在我**自己的**用户空间中，set和get都是自己家事。更细的，栈上方的环境变量是个**字符串常量**一般是只读的，因此如果set修改进程的环境变量，是在**堆区**开辟内存存放新的字符串。

![image](https://user-images.githubusercontent.com/55400137/150061947-1037cfc6-b25f-43a2-b0c6-5f23f7a1b907.png)

更准确详细的内存布局：
![image](https://user-images.githubusercontent.com/55400137/150064134-9f00848c-1a1a-4ffb-a53c-1ba5406fbc5b.png)

**注意**：在.data段中还可以分为RO data只读数据段和RW data可读写数阶段，只读数据段包括全局const常量和字符串常量。

* **内核区**
这部分有个文章很清楚:[从环境变量到整个Linux进程运行](https://www.zhyingkun.com/markdown/environ/)

通过`ps`命令查看当前进程快照，`ps asf`命令查看所有进程的详细信息以及关系。**ps即`process snapshot`进程快照**
然后可以用`pmap [pid]`命令查看进程号为pid的进程的**内存图**，**pmap即`process memory map`进程内存图**
`top`命令：显示进程的**实时**状况，好像Windows的资源管理器。`ps`命令展示的是进程**快照**，是某一刻的状态。
比如通过下面的代码来验证setenv函数是不是真的会在堆区开辟内存：
```cpp
#include<stdio.h>
#include<stdlib.h>
extern char **environ;
int main()
{
	printf("before set: %p\n", getenv("PATH"));
	setenv("PATH", "asdadasdadadasdasdasdasdadasdasda", 1);
	printf("after set: %p\n", getenv("PATH"));
	getchar(); //让进程停住
	exit(0);
}
```
输出如下:
```cpp
before set: 0x00007fff2a092e68
after set: 0x000056011372a675
```
先通过`ps awf`查看进程号后再`pmap`查看内存空间如下：
```cpp
0000560111d07000      4K r-x-- test //可读可执行，代码段
0000560111f07000      4K r---- test //只读数据段
0000560111f08000      4K rw--- test //可读写数据段
000056011372a000    132K rw---   [ anon ]
00007f7862b62000   1948K r-x-- libc-2.27.so
00007f7862d49000   2048K ----- libc-2.27.so
00007f7862f49000     16K r---- libc-2.27.so
00007f7862f4d000      8K rw--- libc-2.27.so
00007f7862f4f000     16K rw---   [ anon ]
00007f7862f53000    164K r-x-- ld-2.27.so
00007f7863163000      8K rw---   [ anon ]
00007f786317c000      4K r---- ld-2.27.so
00007f786317d000      4K rw--- ld-2.27.so
00007f786317e000      4K rw---   [ anon ]
00007fff2a072000    132K rw---   [ stack ] //栈
00007fff2a14d000     12K r----   [ anon ]
00007fff2a150000      4K r-x--   [ anon ]
ffffffffff600000      4K r-x--   [ anon ]
 total             4516K
```
显然原来的环境变量地址在栈的上方，设置后的环境变量地址在可读写数据段的上方，也就是**堆**。
### 关于库
常用的几种库：动态库，静态库，共享库(手工装载库)。
实际上动态库和共享库是一个东西：
* Linux上叫共享库，用.so结尾；
* Windows上叫动态库，结尾是典中典的.dll.
有手工装载库的一套标准函数：
```cpp
void *dlopen(const char *filename, int flags); //用flags方式打开一个共享库文件，并返回一个句柄
int dlclose(void *handle); //关闭共享库
void *dlsym(void *handle, const char *symbol); //在共享库中寻找符号symbol，并返回入口地址
```
下面是给的例子，手工加载数学库(用LIBM_SO宏表示)并计算cos
```cpp
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <gnu/lib-names.h>  /* Defines LIBM_SO (which will be a
			      string such as "libm.so.6") */
int main(void)
{
   void *handle;
   double (*cosine)(double);
   char *error;

   handle = dlopen(LIBM_SO, RTLD_LAZY);
   if (!handle) {
       fprintf(stderr, "%s\n", dlerror());
       exit(EXIT_FAILURE);
   }

   dlerror();    /* Clear any existing error */

   cosine = (double (*)(double)) dlsym(handle, "cos");  
   error = dlerror();
   if (error != NULL) {
       fprintf(stderr, "%s\n", error);
       exit(EXIT_FAILURE);
   }

   printf("%f\n", (*cosine)(2.0));
   dlclose(handle);
   exit(EXIT_SUCCESS);
}
```
注意`dlsym`在库中寻找`cos`函数的入口地址后，返回的是`void*`万能指针，直接强制类型**转换成函数指针**就OK啦。

### 函数的跳转
比如一个树形结构我们**寻找**一个结点用递归函数递归了一万层，那么我们其实没有必要再一层层退栈，这样效率太低了。

而goto**关键字**不能跨函数跳转，不能跨函数实际上就是**不能跨函数栈**，而递归函数的每一层都有自己单独的栈，因此也不行。

C++的异常处理机制倒是可以避免一层层退栈，但我们这里讲的是C。

`setjmp()`和`longjmp()`提供了`nonlocal goto`，用于实现**跨函数的安全跳转**。
`setjmp()`设置跳转点，也就是说其他语句**向这个地方跳**。因此有两个返回值，如果是刚设置跳转点的话setjmp返回0，从longjmp**跳回**的话返回值为非零。另一个类似两种返回值的函数是`fork()`。
原型：
```cpp
int setjmp(jmp_buf env);   //需要设置一个jmp_buf类型的全局变量
void longjmp(jmp_buf env, int val); //跳回到setjmp处时带一个返回值val
```
因此可不可以让longjmp的val返回一个0？手册上明确提到，如果传入0，那么将变成1.
```cpp
static jmp_buf save;
void b()
{
	printf("%s begin\n", __FUNCTION__);
	printf("%s jump!\n", __FUNCTION__);
	longjmp(save, 6);
	printf("%s end\n", __FUNCTION__);
}
void a()
{
	printf("%s begin\n", __FUNCTION__);
	printf("%s call b", __FUNCTION__);
	b();
	printf("%s return from b", __FUNCTION__);
	printf("%s end\n", __FUNCTION__);
}
int main()
{
	printf("%s begin\n", __FUNCTION__);
	int ret = setjmp(save);   //设置跳转点
	if(ret == 0)
	{
		printf("%s call a", __FUNCTION__);
		a();
		printf("%s return from a", __FUNCTION__);
	}
	else
	{
		printf("jump here code %d\n", ret);
	}
	exit(0);
}
```
### 进程资源的获取和控制
`ulimit -a`**命令**查看允许**单个进程**获取的各种资源上限。

**系统调用函数**`getrlimit()`和`setrlimit()`，这两个函数一封装就成了`ulimit`命令


# 进程相关
## 进程的基本内容
### 进程标识符:`pid`
* 进程号pid的类型是`pid_t`，一般是个有符号的16位整型数。进程号**是顺次向下使用**，而不是像文件描述符fd那样优先使用最小的。
* 重要命令查看进程快照:`ps`命令。常用选项
	* `axf`
	* `axm`，m表示more，即更详细的信息
	* `ax -L`，用Linux特有的方式进行查看
* 系统调用查看进程号：
	* `pid_t getpid()` 返回当前进程的pid
	* `pit_t getppid()`  返回当前进程父进程的pid
### 进程的产生：`fork()`函数
`fork()`用**复制当前进程**的方式创建一个子进程，**执行一次返回两次**的函数典范，另一个就是上面提到的`setjmp()`函数。
所谓复制就是全员拷贝，包括**当前程序的执行位置**都一样，也就是执行完fork()后，**父子进程都在`fork()`这一句返回**。然后我们根据返回值的不同来判断究竟是父还是子。
#### fork()实例1
* fork()函数之后父子进程谁先运行取决于**调度器的调度策略**，想让某个进程先跑可以采用`sleep(1)`这样的简单策略
```cpp
int main()
{
	printf("[%d]:begin!\n", getpid());
	fflush(NULL); //!!!!! fork之前一定要加上刷新流的操作，后面有详细的案例！
	int ret = fork();
	if(ret < 0)
	{
		perror("fork()");
		exit(1);
	}
	else if(ret == 0) //child
	{
		printf("[%d]: child working!\n", getpid());
	}
	else //parent
	{
		//sleep(1); //让父进程睡一会
		printf("[%d]: parent working!\n", getpid());
	}
	printf("[%d]: end!\n", getpid());
	getchar(); //让程序不停止
	exit(0);
}
```
上面代码执行后用`ps axf`看看进程关系如下：
```cpp
  PID TTY      STAT   TIME COMMAND
 1612 ?        Ss     0:00 /lib/systemd/systemd --user
 2162 ?        Ssl    0:23  \_ /usr/lib/gnome-terminal/gnome-terminal-server
 2171 pts/0    Ss     0:00  |   \_ bash
 3394 pts/0    T      0:00  |   |   \_ ./test
 3874 pts/0    R+     0:00  |   |   \_ ps axf
 2180 pts/1    Ss     0:00  |   \_ bash
 3869 pts/1    S+     0:00  |       \_ ./fork1
 3870 pts/1    S+     0:00  |           \_ ./fork1
```
* 可以看到**bash进程**创建了父进程，父进程又创建了子进程，父子进程名字**完全一样**。
* 从图中可以看出父子进程关系，而对于所有**顶格写**的父进程是init进程。比如`/lib/systemd/systemd --user`的父进程就是**init**，可以通过命令`ps -ef`直接查看PPID。
* init进程fork出了很多子进程，这些子进程再**自己**去fork新的子进程，而不是所有的进程都是init进程fork出来的，因此只把它称为祖先进程

#### fflush的重要性
现在我们讨论更深入的问题：上面的程序输出：
```cpp
[3869]: begin
[3869]: parent working!
[3869]: end!
[3870]: child working!
[3870]: end!
```
这是显然的，虽然顺序可能不同，但是可以确定：begin只打印一次而end打印两次。**然而**：
如果`./fork1 > /tmp/out`将输出重定向到文件里，却会发现文件内容为：
```cpp
[3869]: begin
[3869]: parent working!
[3869]: end!
[3869]: begin
[3870]: child working!
[3870]: end!
```
begin打印了两次？？？实验结果确实是这样的，肯定想到和标准IO的流缓冲区有关，但是我们的printf都煞费苦心加上了`\n`了啊。还是没懂！

* stdout流是**行缓冲**，但是我们重定向到文件流里，普通文件流都是**全缓冲**的，因此父进程的`[3869]: begin`放到缓冲区里没有写到文件里，fork之后被**完全复制到子进程**，之后父子进程各自的缓冲区都是以`[3869]: begin`开头加上后面的内容，输出到文件中，所以有两句begin。

* 因此有个[**重要的经验**]：在fork之前，一定要**先fflush(NULL)** 刷新所有已经打开的输出流。

#### fork()实例2
想要fork出多个子进程，要特别注意避免子进程也去fork，形成二叉树式的进程增长，即要**让子进程及时exit掉**。
比如下面的例子我们想要fork出10个进程，但实际上每个子进程都在fork，最后生成了1024个进程：
```cpp
int main()
{
	int i = 0;
	for(;i<10;i++)
		fork();
	exit(0);
}
```
而让子进程exit()就会引出其他的问题：**僵尸进程**和**孤儿进程**，比如下面的程序：
```cpp
#define LEFT 30000000
#define RIGHT 30000200
int main()
{
	int i = 0, j = 0;
	pid_t pid;
	int mark = 0;
	for(i = LEFT; i<RIGHT; i++)
	{
		pid = fork(); //父进程创建出200个子进程
		if(pid < 0)
		{
			perror("fork()");
			exit(1);
		}
		if(pid == 0) //子进程执行计算
		{
			mark = 1;
			for(j = 2; j<i/2;j++)
				if(i % j == 0)
				{
					mark = 0;
					break;
				}
			if(mark == 1)
				printf("the primer is %d\n", i);
			//sleep(1000); //case 1: 子进程睡醒发现父亲已经死了，变成了孤儿
			exit(0); //为了避免指数fork让子进程及时终止
		}
	}
	//sleep(1000); //case 2: 父进程做梦时孩子死完了，并且都变成了僵尸
	exit(0);
}
```
显然创建出的200个子进程**并发执行**，最后的输出结果不是按顺序的。

* case 1: 孤儿进程
接下来如果让每个子进程在exit之前sleep(1000)，显然运行状况应该是子进程在睡觉，父进程却死了。这时候通过`ps axf`查看进程关系：
```cpp
4371 pts/1    S      0:00  \_ ./primer1
4372 pts/1    S      0:00  \_ ./primer1
4373 pts/1    S      0:00  \_ ./primer1
4374 pts/1    S      0:00  \_ ./primer1
.
.
.
```
我们发现，所有的子进程都顶头写。也就是说他们的**父进程变成了init进程**，创建他们的父亲已经正常exit结束掉了，也就是说父进程死了之后他们变成了孤儿，**孤儿进程由init进程接管**。
init进程接管这些孤儿的时候，孤儿都在sleep，只不过父亲变了，子进程再执行exit()正常终止，内核释放其资源并且init进程收尸，完整结束。
inti进程相当于**孤儿院**，可以接管所有的孤儿，因此实际上孤儿进程并没有什么危害。

可以看到这些子进程的状态都是`S`，是可被打断的睡眠，还在睡眠中。各种状态的描述如下：
```cpp
D    uninterruptible sleep (usually IO)
R    running or runnable (on run queue)
S    interruptible sleep (waiting for an event to complete)
T    stopped by job control signal
t    stopped by debugger during the tracing
W    paging (not valid since the 2.6.xx kernel)
X    dead (should never be seen)
Z    defunct ("zombie") process, terminated but not reaped by its parent
```
可以通过`killall primer1`来杀死所有名为primer1的子进程。

* case 2: 僵尸进程
首先资源释放有个基本的原则：**谁创建谁释放，谁打开谁关闭**。子进程死亡之后，内核会释放掉子进程的所有资源，包括内存空间，打开文件等，但是仍会保留一定的信息，比如进程号、退出状态、运行时间等信息，这些**残留信息只能通过父进程释放**。

因此僵尸进程一个明显的**危害**就是：僵尸会**占用进程号**，导致可以创建的进程数变少。

这里让父进程exit前sleep，孩子在睡梦中都exit死掉了，但是由于父进程孩子sleep，无人给孩子们**收尸**，他们都变成了**僵尸进程**。
这次的结果为：
```cpp
2180 pts/1    Ss     0:00  |   \_ bash
4623 pts/1    S+     0:00  |       \_ ./primer1
4624 pts/1    Z+     0:00  |           \_ [primer1] <defunct>
4625 pts/1    Z+     0:00  |           \_ [primer1] <defunct>
4626 pts/1    Z+     0:00  |           \_ [primer1] <defunct>
.
.
.
```
可以看到父进程状态为`S+`还在睡觉，但是他的子进程全部exit死掉了，并且无人收尸变成了僵尸进程`Z+`。一直等到父进程死后，这些进程变成了孤儿进程由init进程接管，释放资源。
而僵尸进程是**不能**通过kill命令杀死的，因为他们早就exit()死掉了。
也就是说，一般来讲，僵尸进程的**存在时间都很短**，因为只要被init进程接管后资源就会被释放。

比较僵尸进程和孤儿进程，他们最终**都是由init进程所释放**的，孤儿进程直接换爹被init接管，僵尸进程也是在父亲死后被init进程所接管。

#### fork之后父子进程的区别
上面提到了就是全员复制创建新进程，连执行位置都一样。但是还是有一些不一样的地方：
* 返回值不一样：如果成功在父进程中fork返回子进程的id号，子进程返回0.如果返回-1表示没有生成子进程并且设置errno。
* 父子进程的pid和ppid不同
* 未决信号和文件锁不继承：未决信号即还没来得及响应的信号
* 资源利用量清零


#### 写时拷贝技术
如果父进程的内存太大，那么完全拷贝一份代价过高，于是就有了`vfork()`函数:让父子进程**共用**同一份资源，不管是否修改，都不会拷贝一份新的。可以把vfork()看作**浅拷贝**。
而原始的fork()函数就是**深拷贝**。
现在的fork引入了**写时拷贝**技术，是**深拷贝的加强**，也就是一种**延迟拷贝**：
* 父子进程的共享内存空间，如果某个进程对内存进行了修改，那么执行写操作的这个进程就会自己拷贝一份数据放到另一块地方，防止干扰到另一个进程。
* 由于只读的话相当于多个指针指向同一块内存，因此为了避免类似浅拷贝的多次释放的问题，增加了**引用计数**的概念，通过指针释放内存时引用计数--，为0时才真正释放。
  > Linux在很多地方都用到了引用计数，比如文件系统的**硬链接文件**和原文件指向同一个inode结点，因此只有当inode上的**引用计数**为0时，才真正释放inode。

### 进程的消亡及资源的释放
`wait()`和`waitpid()`系统调用：挂起父进程，直到有一个子进程状态发生变化(一般指终止)，并将状态信息存到wstatus指向的内存中。
一般用于检测子进程终止后**给子进程收尸**，防止僵尸进程存在时间过长。
```cpp
pid_t wait(int *wstatus);
pid_t waitpid(pid_t pid, int *wstatus, int options); //等待子进程号为pid的状态变化
options可以选择NOHANG，不再死等，如果没有收到子进程pid的exit则立即结束自己。也就是不再阻塞而是非阻塞
//下面两种方式等价
wait(&wstatus);  //死等所有的子进程
waitpid(-1, &wstatus, 0); //-1表示等待所有子进程
```
wstatus指向一个int，可以用下列宏来检测状态：
```cpp
WIFEXITED(wstatus) 如果正常终止返回true
WEXITSTATUS(wstatus) 返回退出状态
WIFSIGNALED(wstatus) 如果子进程被信号终止返回true
WTERMSIG(wstatus) 如果上一条为真，这一条返回那个信号的数字
WIFCONTINUED(wstatus) (since Linux 2.6.10) 如果子进程被SIGCONT恢复返回true
...等等
```
使用方法就在父进程结束前加上:
```cpp
int st;
for(i=LEFT; i<RIGHT;i++)
	wait(&st); //等待200个子进程都结束
```
### 进程分配法
之前的例子中我们创建了200个子进程，让每一个子进程算一个数。可不可以只创建3个子进程，让每个进程算的数字多一点，这样还能省下不少pid资源。
于是就涉及到了进程分配的问题，最简单的方法就是**分块法**，让第一个进程算前1/3，第二个算中间1/3，第三个算后1/3。
但是由于前1/3的质数最多，因此负载最大，我们总想**让每个子进程的工作量大致相等**，这里提一种**交叉分配法**，当然还有更好的进程池法。
交叉分配法就像**发牌**一样，200个任务(牌)轮流分配个三个进程(人)，实现如下：
```cpp
#define N 3
for(i = 0; i< N;i++)
{
	pid = fork();  //父进程创建三个子进程
	if(pid == 0)
	{
		for(j = LEFT+i; j<=RIGHT; j+=N) //每个子进程任务不同
		{
			...
		}
		exit(0); //子进程结束
	}
}
```
### `exec()`函数族
延续之前的例子，我们直到fork是复制出来一个子进程，写时拷贝是深拷贝的加强，进程关系如下：
```cpp
\_ bash
    \_ ./primer1
        \_ [primer1]
        \_ [primer1]
        \_ [primer1]
```
./primer1是bash进程fork出来的，那为什么bash的子进程不是bash，而变成了./primer1？这就是`exec()`函数族的作用:
```cpp
The  exec() family of functions replaces the current process image with
a new process image.
```
它们的作用就是用一个新的进程映像**替换**当前进程，也就是说**pid不变**。
函数族声明如下：
```cpp
extern char **environ;

int execl(const char *path, const char *arg, ... /* (char  *) NULL */);  
//path要求进程映像的完整路径，之后的arg是所带的选项，好比`ls -a -i`，注意从argv[0]开始，最后用NULL做结尾
int execlp(const char *file, const char *arg, ... /* (char  *) NULL */);  
//file只要求文件名，程序会在环境变量的PATH里挨个找
int execle(const char *path, const char *arg, ... /*, (char *) NULL, char * const envp[] */);
int execv(const char *path, char *const argv[]);	
int execvp(const char *file, char *const argv[]);
```
* exec函数族如果成功什么都不返回，因为已经**变成别的进程了**，永远不会回来，失败的话返回-1并设置erron。
* execl的argv需要从argv[0]开始，但是实际上这个名字不重要，exec进程名称是什么都可以，重要的是进程**映像的路径**才是执行的东西
例子：
```cpp
int main()
{
	puts("Begin!");
	// fflush(NULL);
	execl("/bin/date", "date", "+%s", NULL);
	puts("End!");
	exit(0);
}
```
最后输出`Begin!`和一个数字，不会输出`End!`，因为execl之后进程已经变成date进程了。
#### 再谈fflush()的重要性
像fork函数中提到的那样，输出重定向到文件中`./exec > /tmp/out`，发现文件里只有个数字，连`Begin!`都没了。
这是因为普通文件流是**全缓冲**，`Begin!`还没来得及输出进程就被换成date了，因此只有个数字，解决方式还是：在`exec()`之前**刷新所有的输出缓冲**`fflush(NULL)`

#### 整体来看
有个问题？一个进程何必再exec成其他进程？既然这样为什么不一开始就执行那个进程？要知道的是我们是基于`fork`来创建进程的，因此只能想着把复制来的进程**变成**其他进程，这就是`exec()`。
下面再给个完整的例子：
```cpp
int main()
{
	puts("Begin!");
	fflush(NULL); //fork之前刷新输出
	int pid = fork(); 
	if(pid < 0)
	{
		perror("fork()");
		exit(1);
	}
	if(pid == 0) //子进程
		execl("/bin/date", "+%s", NULL);
	wait(NULL); //父进程等着子进程收尸
	puts("End!");
	exit(0);
}
```
这就完成了fork出一个子进程，并且子进程execl摇身一变，变成了其他进程。整个**Linux世界的运作机制就是这样的**。

### shell执行命令的完整过程
shell输出提示信息，**循环**接受命令执行命令。在shell中输入命令`ls`按下回车：shell进程执行fork创建出子进程shell，这个子进程执行`exec()`摇身一变变成ls进程，这时的父进程shell在**wait等待**子进程的exit好给它收尸，收尸之后shell重新输出提示信息。

* 这里就涉及到孤儿进程的案例中的一个输出细节：

  在孤儿进程的案例中，实际上我们发现并不是输出完所有的质数然后再输出命令行提示信息，而是**先输出**了命令行提示信息，之后才输出了所有质数。
* 这是因为：
  
  shell进程的子进程是./primer1记为A，这个进程再创建了200个子进程。shell进程一直wait**等待A进程**的结束，A一结束就收尸并且输出提示信息。在案例中A进程早早就exit了，留下一堆孤儿。
  
* 那么更深入的：

fork出来的子进程凭什么和父进程输出到同一个终端里？这是因为子进程和父进程的文件描述符表一样，因此**共用一个**系统打开表中的结构体，前三个stdin, stdout, stderr当然也一样了。

那么可以想到一个简单的**父子进程通信**的方式：毕竟共用一个系统打开表的结构体，因此可以一个读一个写。

#### 一个简单shell实现
见myshell.md和myshell.c，模拟了shell处理外部命令的全过程。算是对fork,exec和wait的综合练习。

### 用户权限及组权限
之前的文件系统中讲到文件的**所有者**用户ID和组ID，即：st_uid和st_gid，这是文件的属性。

而与一个**进程相关联**的ID有6个或者更多，即：r e s，(r)实际用户ID实际组ID，(e)有效用户ID和有效组ID，(s)保存的设置用户ID和保存的设置组ID。
一个进程执行文件操作时，一般是将本进程的**有效ID**和文件的所有者ID进行比对。
* 每个用户的进程有自己相关的ID，进程执行文件操作时需要进行自己的ID和文件所有者ID的对比。
* root用户进程的有效用户ID是0，对整个文件系统都有最大的自由
* 这里说一下从init进程到shell进程的权限变化：
	* 程序启动后，第一个进程init，此时还是root用户身份，ID为0.
	* 之后init进程fork exec产生一个getty进程，提示输入用户名
	* 这个getty进程**没有fork**，而是直接exec变成login进程，让你输入密码。**到此为止，这些进程都是root用户身份**执行的。
	* 得到用户名和密码进行密码校验，就是之前文件系统提到的加密函数得到密文后与/etc/passwd里的串进行对比。
	* 如果验证通过，那么login进程fork/exec产生**shell进程**，并且把身份**切换成用户的res**。
		* 切换身份就是下面提到的`setuid()`系列函数，设置当前进程的各种ID，由于**执行setuid时还是root**身份，因此可以。
		* 而如果一个普通进程想要setuid改变身份时，需要**以root身份执行**这个进程才行。

#### 相关函数
```cpp
uid_t getuid(void); //返回当前进程的实际用户ID
uid_t geteuid(void); //返回当前进程的有效用户ID

int setuid(uid_t uid); //设置当前进程的有效用户ID
int setgid(gid_t gid); //设置当前进程的有效组ID

gid_t getgid(void); //返回当前进程的实际组ID
gid_t getegid(void); //返回当前进程的有效组ID
```

可以使用`sudo chown root filename`命令更改filename文件的**所有者**为root(chown必须root用户才能执行)。

### 扩展：解释器文件
即**脚本文件**。
* 脚本文件开头有标记：`#!/bin/bash`，也就是使用的**脚本解释器**映像文件，和/etc/passwd里的用户登录shell一样
	* 因此这个文件不一定非得要`/bin/bash`或者什么`python`，而是**随便一个可执行文件**就可以，比如`#!/bin/cat`当然可以
* 然后用这个解释器，解释下面的一行行写命令
* 一般写下的脚本文件没有可执行权限，因此`chmod u+x file`授予一下，然后`./file`就一行行去执行了

解释过程：
* 当`./file`时候，看到第一句的脚本标记`#!/bin/bash`，会装载这个映像文件
* 然后由这个映像文件解释整个文件内容，由于shell中`#`是**注释标记**，因此第一行就**忽略**掉了，直接解释下面的一行行命令。

如何证明？我们就把解释器写成`#!/bin/cat`，文件如下：
```cpp
#!/bin/cat
ls
hhhhh
cat /etc/shadow
```
这样就是用cat程序解释整个文件，在cat看来第一句**不是注释**，因此**全员打印**，把整个文件都输出：
```cpp
#!/bin/cat
ls
hhhhh
cat /etc/shadow
```

### `system()`函数
system()标准函数用来**执行一个shell命令**，他的过程也是典中典：fork + execl + wait的封装
```cpp
int system(const char *command); //执行一个shell命令

DESCRIPTION
The  system()  library  function uses fork(2) to create a child process
that executes the shell command specified in command using execl(3)  as
follows:

   execl("/bin/sh", "sh", "-c", command, (char *) 0);

system() returns after the command has been completed.
```
从exec就能看出，这玩意纯纯的内部调用`/bin/sh`来执行command，和命令行直接输入`sh -c 命令`一样。参数用NULL结束，就是那个`(char *)0`

这和我们写的myshell不一样，这个是用shell来实现shell，我们那个是完整的模拟。而且这玩意和myshell内部也就是在`execl`的参数有点不同，其余完全一致hhh

### 进程会计和进程时间
`acct()`函数，是个方言函数，不支持POSIX标准(Unix标准)，方言BSD或者systemV啥的。

之前测试进程时间都是用`time`命令，这里还有个`times()`函数，它是time的中之人。
```cpp
clock_t times(struct tms *buf);
struct tms {
	clock_t tms_utime;  /* user time */
	clock_t tms_stime;  /* system time */
	clock_t tms_cutime; /* user time of children */
	clock_t tms_cstime; /* system time of children */
};
```
可以看到tms结构体里面存放自己的用户态时间和内核态时间，以及**子进程们**的时间。

time命令显示的用户时间和内核时间就是**自己的加上子进程们的时间**。**real时间**再加上一些调度的时间等等。
### 守护进程

### 系统日志的书写

## 并发
## 信号
多进程并发(信号量)
多线程并发
## 线程
## 数据中继
## 高级IO
## IPC(进程间通信)
