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
比如kill命令，发信号杀死进程
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
* 守护进程脱离了控制终端，常常在系统引导装入时启动，在关机时才终止，是一种**长期生存**的进程。由于**没有控制终端**，因此是在**后台运行**。
#### 一些概念
* 会话：session，每个会话都有一个**会话首进程**，表示为`sid`。会话是多个**进程组**的集合，进程组可分为：**一个前台**进程组和**多个后台**进程组，前台进程组可以进行终端设备读写。每个进程组都有个**组长进程**。
	* 通常是由shell的**管道**操作将几个进程编成一组的，比如`proc1 | proc2`则这两个进程为一组。
	* 一个**非进程组组长**进程可调用`pid_t setsid(void);`函数来创建一个**新的会话**
		* 则该进程变为此会话的会话首进程，即SID = PID
		* 该进程成为一个新进程组的组长进程，进程组ID就是此进程的ID，即PGID = PID
		* 并且该进程**脱离控制终端**。
		* 如果一个组长进程试图调用该函数，则返回出错。因此想调用必须fork出子进程，**子进程和父进程同属一个进程组**，并且子进程ID是新分配的，**组长还是父进程**。
	* 
* 终端：我们用的terminal都是虚拟终端，真正意义上的终端是在上世纪计算机特别昂贵时，一台计算机对应好几个终端。‘

#### 守护进程的特点
使用`ps axj`命令查看各个进程状态：`x`表示显示无控制终端的进程。
```cpp
PPID   PID  PGID   SID TTY      TPGID STAT    UID   TIME  COMMAND
0     1     1     1     ?         -1   Ss       0   0:01  /sbin/init splash
0     2     0     0     ?         -1   S        0   0:00  [kthreadd]
2     56    0     0     ?         -1   S        0   0:00  [kswapd0]

```
其中：
PID = 进程ID （由内核根据延迟重用算法生成）
PPID = 父进程ID（只能由内核修改）
PGID = 进程组ID（子进程、父进程都能修改）
SID = 会话ID（进程自身可以修改，但有限制，详见下文）
TTY = **控制终端名称**
TPGID= 控制终端进程组ID（由控制终端修改，用于指示当前前台进程组）
UID = 进程用户ID，0表示root用户

客户端向服务器连接后，创建一个**会话**，展示**登录shell**，这就是个**会话**，在这个会话里面**shell是个进程组**，之后我们输入:`ps axj | grep 2288`得到如下：

![image](https://user-images.githubusercontent.com/55400137/153831727-c5bce5b3-82cf-4b46-8b15-35d1cb25e83c.png)


可以看到shell确实是个**单独进程组**，PID = PGID = SID，而`ps`和`grep`这两个进程**父进程都是shell**，在一个进程组PGID=3463中，而shell, ps, grep这三个进程在**同一个会话**SID=2288中。

显然如果我们想自己创建一个用户守护进程，必须新建会话，因为shell这个会话必定**有控制终端**，只能另起炉灶即`setsid`函数；并且由于setsid不能由进程组组长调用，因此只能fork**子进程**调用，详细实现见下文。

他们的控制终端都是`pts/0`，可以通过`tty`命令查看得到：`/dev/pts/0`，这个控制终端是和shell界面对应的。。不是什么真真意义上的终端，也就是说你再创建个shell界面tty就成了`pts/1`，多创建几个就依次是`pts/2, pts/3...`，比如我们创建三个shell界面，那么在第三个shell里输入`ps axj | grep bash`就会得到：

![image](https://user-images.githubusercontent.com/55400137/153833779-e8e46f50-5131-4455-ad18-2b5ecf533246.png)

从这上面看，差不多就是创建一个新的shell界面就有个**对应的新会话**，还是很好懂的。


* 父进程ID为0的进程通常是内核进程，init进程例外。
* 在ps的输出中，**内核守护进程**的名字用**方括号**括住。比如kswapd就是**内存换页**守护进程。
* 大多数守护进程都是以root特权运行，UID=0，并且**所有**守护进程都**脱离终端控制**，因此**tty=?**
	* 这是因为**内核守护进程**都以无控制终端方式启动，
	* **用户守护进程**调用setsid函数会脱离终端。
下面两个就是内核守护进程：
```cpp
0     2     0     0     ?         -1   S        0   0:00  [kthreadd]
2     56    0     0     ?         -1   S        0   0:00  [kswapd0]
```
* **用户**守护进程的**父进程是init进程**，即**PPID=1**。
	* 因为只能是子进程调用setsid，并且显然我们不能再让父进程去wait守护进程(守护进程长期存在)，因此就像孤儿进程那样让直接**由init进程接管**。
* 用户守护进程的`PID = PGID = SID`，同样由于setsid函数的作用

下面两个是用户守护进程：
```cpp
1   539   539   539 ?           -1 Ssl      0   0:00 /usr/lib/udisks2/udisksd
1   599   599   599 ?           -1 Ss       0   0:00 /lib/systemd/systemd-logind
```

单实例守护进程：锁文件`/var/run/filename.pid`

系统守护进程的启动脚本文件：`/etc/rc*.d`

#### 一些函数
除了上面的`setsid()`函数，还有几个和进程组有关的函数：
```cpp
int setpgid(pid_t pid, pid_t pgid);
pid_t getpgid(pid_t pid);
pid_t getpgrp(void);                 /* POSIX.1 version */
int setpgrp(void);                   /* System V version */
```

#### 实现一个守护进程
一个重要的原则就是守护进程脱离终端，因此不能再用标准输入输出了。
```cpp
int make_protect()
{
	pid_t pid;
	pid = fork();
	if(pid < 0)
		return -1;
	if(pid > 0) // parent
		exit(0);
	//child call setsid and cut down terminal
	int fd = open("/dev/null", O_RDWR);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	setsid();
	chdir("/"); //切换工作目录为根目录
	return 0;
}
int main()
{
	int  i;
	FILE *fp;
	openlog("nytest", LOG_PID, LOG_DAEMON);
	
	fp = fopen("/tmp/out", "w");
	if(fp == NULL)
	{
		syslog(LOG_ERR, "fopen:%s", strerror(errno));
		exit(1);
	}
	if(make_protect() < 0)
	{
		syslog(LOG_ERR, "daemon fail!"); //提交日志
		exit(1);
	}
	else
	{
		syslog(LOG_INFO, "darmon success!");
	}
	for(i = 0;;i++)
	{
		fprintf(fp, "%d", i);
		fflush(fp);
		syslog(LOG_DEBUG, "%d is printed", i);
		sleep(1);
	}
	closelog();
	exit(0);
}
```
向文件`/tmp/out`中一秒写一个数字，进程最后`ps axj`查看变成了**后台**进程，用`tail -f /tmp/out`可以看到文件一直在变化

![image](https://user-images.githubusercontent.com/55400137/150498690-3fff39af-0ced-4c44-b830-cf34cbab42d5.png)

可是这个父进程也不是init进程1啊？查看那个进程发现父进程也是个守护进程，因此ubuntu的实际操作可能是先创建了一个守护进程，这个守护进程再fork依次。
```cpp
1  1590  1590  1590 ?           -1 Ss    1000   0:00 /lib/systemd/systemd --user
```

当然Linux也提供了创建守护进程的系统调用：
```cpp
int daemon(int nochdir, int noclose); //nochdir为0则改变工作目录为根，noclose为0则重定向stdin,out,err到/dev/null
```
更多的实现用户守护进程细节见文章:[守护进程](https://xie.infoq.cn/article/c3a40c88f95d3a1cb56045dc4)
### 系统日志的书写
syslogd服务和一些函数：
```cpp
openlog()
syslog()
closelog()
```
实现在守护进程的那个例子中，其实也是为了彻底和终端切断，所以状态信息都输出到日志里。

日志文件在`/var/log/`文件夹下，上面的程序写到了`/var/log/message`文件里，可以用root身份查看。


## 并发
* 同步和异步，同步是**阻塞**模式，异步是**非阻塞**模式。
	* 同步就是指一个进程在执行某个请求的时候，若该请求需要一段时间才能返回信息，那么这个进程将会一直等待下去，知道收到返回信息才继续执行下去，这样便于管理，但是资源利用率非常低

	* 异步是指进程不需要一直等下去，而是继续执行下面的操作，不管其他进程的状态。当有消息返回式系统会通知进程进行处理，这样可以提高执行的效率。

### 信号实现并发
#### 信号的概念
信号是**软件**的中断，信号的响应依赖于中断。

可以通过`kill -l`命令查看所有信号：
```cpp
 1) SIGHUP	 2) SIGINT	 3) SIGQUIT	 4) SIGILL	 5) SIGTRAP
 6) SIGABRT	 7) SIGBUS	 8) SIGFPE	 9) SIGKILL	10) SIGUSR1
11) SIGSEGV	12) SIGUSR2	13) SIGPIPE	14) SIGALRM	15) SIGTERM
16) SIGSTKFLT	17) SIGCHLD	18) SIGCONT	19) SIGSTOP	20) SIGTSTP
21) SIGTTIN	22) SIGTTOU	23) SIGURG	24) SIGXCPU	25) SIGXFSZ
26) SIGVTALRM	27) SIGPROF	28) SIGWINCH	29) SIGIO	30) SIGPWR
31) SIGSYS	34) SIGRTMIN	35) SIGRTMIN+1	36) SIGRTMIN+2	37) SIGRTMIN+3
38) SIGRTMIN+4	39) SIGRTMIN+5	40) SIGRTMIN+6	41) SIGRTMIN+7	42) SIGRTMIN+8
43) SIGRTMIN+9	44) SIGRTMIN+10	45) SIGRTMIN+11	46) SIGRTMIN+12	47) SIGRTMIN+13
48) SIGRTMIN+14	49) SIGRTMIN+15	50) SIGRTMAX-14	51) SIGRTMAX-13	52) SIGRTMAX-12
53) SIGRTMAX-11	54) SIGRTMAX-10	55) SIGRTMAX-9	56) SIGRTMAX-8	57) SIGRTMAX-7
58) SIGRTMAX-6	59) SIGRTMAX-5	60) SIGRTMAX-4	61) SIGRTMAX-3	62) SIGRTMAX-2
63) SIGRTMAX-1	64) SIGRTMAX	
```
可以看到前31种信号都是**标准信号**；从32到64，即`SIGRTMIN`到`SIGRTMAX`，RT表示real time实时，也就是**实时信号**，将在最后提到。

![image](https://user-images.githubusercontent.com/55400137/150538012-94a58776-de22-403c-8c88-88830f36eaa8.png)

上面是对一些信号的解释，很多信号的默认动作都是：终止+core

终止就是结束进程，core表示在当前工作目录中复制一份**程序终止时的内存映像**，即**出错现场**。

一般出现`core dump 段错误`看不到这个core文件。是因为进程资源`ulimit -a`中设置的`core file size = 0`。需要通过`ulimit -c 10240`来修改，这样段错误后就能生成core文件，可以在这个出错现场上gdb调试。

#### signal()函数
signa()l**注册信号处理行为**，收到某个信号后通过**回调函数**进行对应的处理。函数原型如下：
```cpp
typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler); //第二个参数是个函数指针，返回值也是个函数指针，因此我们常常展开写：
void (*signal(int, void (*func)(int)))(int); //这种写法的好处在于可以避免typedef可能的命名冲突，因为C语言名空间管理很糟糕
```
处理行为有三种情况：
* SIG_IGN，即收到信号后忽略信号，什么都不做
* SIG_DFL，收到信号后采用信号的默认行为，和不写注册函数一样
* 自定义信号处理行为，也就是传入一个函数指针
一个例子：
```cpp
int main()
{
	int i;
	for(i = 0; i< 10;i++)
	{
		write(1, "*", 1);
		sleep(1);
	}
}
```
如果在输出时`ctrl+C`，程序就会终止。
* `ctrl+C`是`SIGINT`**打断信号**的快捷键，默认动作就是终止
* `ctrl+\`是`SIGQUIT`**退出信号**的快捷键，默认动作是终止+生成core。

由于SIGINT默认动作是终止，我们用signal函数注册新的处理行为：
```cpp
void func(int i)
{
	write(1, "1", 1);
}
int main()
{
	int i;
	//signal(SIGINT, SIG_IGN); //忽略打断信号
	signal(SIGINT, func);  //注册信号处理函数
	for(i = 0; i< 10;i++)
	{
		write(1, "*", 1);
		sleep(1);
	}
}
```
注册之后，按下`ctrl+C`会输出回显和感叹号`^C!`这样子，平平无奇。

但是有个诡异的现象：如果一直按住`ctrl+C`不放，我们会发现程序会**很快**输出`*^C!*^C!*^C!*...`这样子，**远远小于10s!**

牢记：**信号会打断阻塞的系统调用！**，也就是说read，open函数在内核态正执行（或者阻塞住了），来了一个信号，那么会直接从内核态切换到用户态去执行信号处理函数，原来的系统调用就**失败了**，会设置errno=EINTR。

这就非常可怕了，因为我们之前讲的**所有IO操作都是阻塞的**(当然之后会涉及到非阻塞IO)，我们看看`open`和`read`的手册，错误会设置`errno`，errno可能为：
```cpp
ERRORS:
//open
EINTR  While  blocked  waiting  to  complete an open of a slow device (e.g., a FIFO; see fifo(7)), the call was
       interrupted by a signal handler; see signal(7).
//read
EINTR  The call was interrupted by a signal before any data was read; see signal(7).
```
因此之前写的程序为了鲁棒性，即**防止**收到打断信号就立刻exit，需要额外处理一下：
```cpp
do
{
	fd = open("file", O_RDONLY);
	if(fd < 0)
	{
		if(errno == EINTR) //如果是假错则继续
			continue;
		perror("open"); //否则为正错
		exit(1);
	}
}while()

while(1)
{
	len = read(fd, buf, SIZE);
	if(len < 0)
	{
		if(errno == EINTR) //是假错则忽略
			continue;
		perror("read"); //否则为正错
		exit(1);
	}
}

while(len > 0)
{
	ret = write(fd, buf, len);
	if(ret <  0)
	{
		if(errno == EINTR) //是假错则忽略
			continue;
		perror("write"); //否则为正错
		exit(1);
	}
}
```
#### 信号的不可靠
在**早期**的Unix中：
* 标准信号会丢失，实时信号不会丢失。
* 而且信号的行为不可靠，即处理完一次信号后之后的信号处理会变成默认行为。

#### 可重入函数
一个可重入的函数简单来说就是可以被中断的函数，也就是说，可以在这个函数**执行的任何时刻**中断它，转入OS调度下去执行另外一段代码，而返回控制时不会出现什么错误。

也就是一次调用还未结束，另一次调用就开始了，对信号的处理函数很可能发生这种情况，因此signal注册的**信号处理函数必须是可重入函数**，否则行为就是不确定的。

比如如果函数里用到了**全局变量**，那么这个函数在不同的地方调用会对全局资源产生竞争，从而导致**丢失修改**等等未知行为。

**所有**的系统调用都是可重入的，部分标准库函数也是可重入的，比如：`memcpy`，而像`rand`就是不可重入的，有`rand_r()`可重入版本。只要一个函数有`_r`版本，那么原版函数就是**不可重入**的，而标准IO由于**共用缓冲区**，因此也不是可重入的，不能用于信号处理函数。

#### 信号响应全过程
内核为每个进程维护了两个**位向量**，一个阻塞信号集即blocking(也叫信号屏蔽字signal mask），一个未决信号集pending，存放在**进程控制块**(PCB)中的结构体中：
一般这两个位向量都是64位，一一对应上文提到的64种信号。

* 信号屏蔽状态字(block)：64bits，包含了那些被进程屏蔽掉的信号：1是屏蔽，0是不屏蔽
* 信号未决状态字(pending)：64bits，它包含了那些内核发送给进程，但还没有被进程处理掉的信号：1是未决，0是不抵达

比如当内核发送一个信号给进程时，它将会修改进程的pending位向量，譬如说，当内核发送一个SIGINT信号给进程，那么它会将进程的pending[SIGINT]的值设置成 1：

![image](https://user-images.githubusercontent.com/55400137/150621332-dced20c3-1148-4ae7-8289-016b2e687889.png)

也就是`pending[i] & (~blocked[i]) == 1`时，进程才处理这个信号。

我们用B表示屏蔽字，用P表示未决信号。响应过程如下：

* 进程的B[i] = 1,P [i] = 0，表示不屏蔽信号i，收到信号后	B[i] = 1, P[i] = 1；之后被中断打断/时间片耗尽/执行系统调用等等**扎入内核态**将**中断现场**(栈和各种寄存器)压入内核栈，然后排队**等待调度**；
	* **信号准确来讲是在从kernel态回到用户态的路上响应的**，一般来说就三种情况：
		* 执行系统调用
		* 处理中断异常，被打断
		* 时间片耗尽
* 被调度后从内核态返回时**检查信号**发现B[i] & P[i] == 1，因此将**返回地址修改**成注册的处理函数的入口地址进行信号处理，处理时设置B[i] = P[i] = 0，即**信号处理时对该信号进行屏蔽**
	* 需要注意的是这时候可以继续收到信号即让P[i]=1，如果多次收到信号i但P[i]只置一次1，就会造成信号丢失
	* 为什么B[i]也需要置成0？明明将P[i]=0就OK了，这个原因将在**线程**里面深入讲解。
* 处理函数结束后重新让B[i] = 1解除屏蔽，然后再次返回内核态**检查信号**重复上面的过程，直到没有信号可处理；
	* 在用户态信号处理完后如何再次返回内核态？进程会**主动**调用sigreturn()**系统调用**返回内核
* 这时返回地址这才变成中断现场的那个真正的返址，程序从**被打断的位置**继续执行。

几个问题：
* 信号从收到到响应的延迟？
这是因为进程收到信号只是改变了pending向量，在**被中断打断**后保存当前运行状态信息**扎入内核态**，然后等待调度，被调度**从内核态返回用户态时**，**这时才**检测pending和blocking这两个位图看看有没有需要处理的信号，延迟就在这里产生。


* 信号是如何被忽略的？
实际上就是把把**信号屏蔽字**上对应位置成0，你不能阻止信号到来修改pending向量，但是可以修改mask永远都不理会这个信号。

* 标准信号为什么会丢失？
很简单，因为信号在进程看来就是个**位图**，不会累加不会计数。就算信号来了一万次，也不过是把pending**对应位置了一万次1**，进程**不及时处理**的话和一次没有区别，前面的9999次就丢失了。因此标准信号也叫**不可靠信号**。

而后来的实时信号就是为了解决标准信号没有严格响应顺序，并且会丢失的问题，引入了sigqueue**队列**来解决的。

这里有个文章讲的还算清楚:[Linux进程信号处理流程](https://www.jianshu.com/p/4fd8e35a6580)

#### 信号常用函数
```cpp
int kill(pid_t pid, int sig); //给进程pid发一个信号sig。pid可以有特殊值比如:
//>0进程号；0和当前进程同进程组的所有进程(组内广播)；-1给所有有权限发送信号的进程发送(除了init进程，全局广播)；<-1发送给进程组号为-pid的所有进程
//errno有不同的值，比如EPERM没有权限发送信号；ESRCH即pid进程或者进程组不存在
int raise(int sig); //给当前进程或者线程自己发信号
//如果是给进程发送信号，等价于`kill(getpid(), sig)`；如果是多线程情况等价于`pthread_kill(pthread_self(), sig);`
unsigned int alarm(unsigned int seconds);
//倒计时seconds秒给当前进程发SIGALRM信号，这个信号的默认操作是终止程序
int pause(void);
//让进程/线程睡眠sleep直到收到一个信号
abort();
//给本进程发送SIGABRT信号，让进程异常终止并产生core文件
system();
//之前已经讲过system函数相当于调用/bin/sh执行命令，但是在有信号参与的程序中，需要屏蔽掉信号SIGCHLD（），忽略信号SIGINT和SIGQUIT（用signal函数注册即可）
sleep();
//在有些平台下sleep是用alarm和pause封装的，而alarm多于一个的话会被覆盖。可以用nanosleep函数或者usleep替代。
```

比如`alarm`函数的例子：
```cpp
int main()
{
	alarm(1);
	alarm(10);
	alarm(5); //如果有多个alarm最后一个alarm之前的都被取消
	while(1); //让程序忙等
	pause(); //让程序sleep等信号
	exit(0);
}
```
程序运行后5秒，显示`闹钟`字样后进程终止。但是这个程序非常傻，因为这是`while`**忙等**，资源全消耗到while等待上了。这就体现出了`pause()`函数的功能，让程序sleep而不是一直死循环。

sleep在一些系统下面是用alarm和pause实现的，因此最好不要滥用sleep，很可能对其他alarm形成干扰。

alarm的优势在于比time函数的精度更好，比如让一个程序跑5s，使用time函数
```cpp
int main()
{
	time_t end = time(NULL) + 5;
	while(time(NULL)  <= end)
		count++;
	printf("%d", count);
}
```
用alarm闹钟函数：
```cpp
int main()
{
	alarm(5); //5s后终止程序
	while(1)
		count++;
	printf("%d", count);	
}
```
但是这样打印不出来东西，因为收到SIGALRM信号程序就**异常终止**了，因此注册个函数：
```cpp
int loop = 1;
void alarm_func(int s)
{
	loop = 0;
}
int main()
{
	signal(SIGALRM, alarm_func); //注册函数
	alarm(5); //5s后终止程序
	while(1)
		count++;
	printf("%d", count);	
}
```
这样用time测出的时间和5s非常接近。

#### 信号集
信号集操作相关函数：
```cpp
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);
int sigdelset(sigset_t *set, int signum);
int sigismember(const sigset_t *set, int signum);
sigset_t类型：信号集类型
```

#### 信号屏蔽字和pending集的处理
* 进程可以手动调整信号屏蔽字，来决定对那些信号忽略或者处理。

相关函数：
```cpp
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
```
使用例：
```cpp
sigset_t set;
sigemptyset(&set);
sigaddset(&set, SIGINT);  //信号集里放入SIGINT信号
sigprocmask(SIG_BLOCK, &set, NULL); //屏蔽信号集中的所有信号，也就是把屏蔽字对应位置成0
sigprocmask(SIG_UNBLOCK, &set, NULL); //解除屏蔽
```
* sigpending函数
乍一看以为是获取所有的未决信号，但是这是个系统调用，也就是说从内核态返回时**先处理**收到的并且未屏蔽的信号，也就是说最后返回的是**收到的被屏蔽的信号集**，因此是信号屏蔽字的字迹。

#### 扩展
```cpp
sigsuspend();
sigaction();
```


#### 实时信号
如果收到来标准信号和实时信号，先响应标准信号。注意标准信号和实时信号的函数用的是**同一套函数**，只不过信号参数不同，并且标准信号**会丢失**，实时信号可以排队，**不会丢失**。

实时信号排队的数量是有上限的，可以用`ulimit -a`查看：
`pending signals      (-i)    15582`

可以用`kill -信号num 进程号`命令向某进程发送信号，比如`kill -40 1700`即向1700进程发送`SIGRTMIN+6`信号。

### 多线程实现并发
#### 线程的概念
一个线程就是一个**正在运行的函数**，各个线程之间是兄弟关系，平起平坐，没有主次之分，也没有调用和被调用关系。多个线程是**共享同一进程的同一块块地址空间**。每个线程有自己的**栈**和**程序计数器**以及**一组寄存器**；**共享**进程中的**代码段**和**堆**以及**常量和静态变量**等等，因此线程间通信要比进程间通信方便点，找个**全局变量**就可以让同一个进程内的线程贡共享数据。而进程之间由于内存空间不同，需要其他的机制才能通信：比如管道，消息队列，共享内存等等，将在之后讲到。

线程并发是先提出标准后完成实现，因此比信号并发更加规范，我们至今遇到的所有函数几乎**都支持线程并发**。线程的标准有很多，比如POSIX线程标准，其中的**线程标识**是`pthread_t`，其中p代表POSIX，thread代表线程。

可以用函数`pthread_equal(pthread_t, pthread_t)`来比较两个线程是否相等；用`pthread_t pthread_self()`返回自己的线程标识。

信号的局限性在信号处理函数中**必须可重入**，也就是说连标准IO都不能用，只能用一些算数操作或者系统调用。而线程处理没有这些限制，条件比信号弱，因此使用起来更方便。

【参考文章】:[线程之间到底共享了哪些资源？](https://cloud.tencent.com/developer/article/1768025)

更深入的：线程和进程其实本不分家，区别只在于资源的共享情况，对应的**clone函数**，相关文章：[Linux线程实现](https://developer.aliyun.com/article/20607)

可以通过`ps axm`查看线程：
```cpp
  PID TTY      STAT   TIME COMMAND
    1 ?        -      0:01 /sbin/init splash
    - -        Ss     0:01 -
    2 ?        -      0:00 [kthreadd]
    - -        S      0:00 -
  593 ?        -      0:00 /usr/lib/udisks2/udisksd
    - -        Ssl    0:00 -
    - -        Ssl    0:00 -
    - -        Ssl    0:00 -
    - -        Ssl    0:00 -
    - -        Ssl    0:00 -
  600 ?        -      0:00 avahi-daemon: running [njucs-VirtualBox.local]
    - -        Ss     0:00 -
```
可以看到每个进程下面至少有一个`--`，即**每个进程至少有一个线程**，也就是说**进程也是一个容器**，如今**编程的单位**已经细化到了线程，处理器**调度的单位**实际上也是线程。

也可以用**Linux的方式**查看`ps ax -L`：
```cpp
  PID   LWP TTY      STAT   TIME COMMAND
 1090  1090 ?        Ssl    0:00 /usr/bin/whoopsie -f
 1090  1110 ?        Ssl    0:00 /usr/bin/whoopsie -f
 1090  1111 ?        Ssl    0:00 /usr/bin/whoopsie -f
 1093  1093 ?        Ssl    0:00 /usr/bin/python3 /usr/share/unattended-upgrades
 1093  1129 ?        Ssl    0:00 /usr/bin/python3 /usr/share/unattended-upgrades
 1324  1324 ?        Sl     0:00 gdm-session-worker [pam/gdm-launch-environment]
 1324  1325 ?        Sl     0:00 gdm-session-worker [pam/gdm-launch-environment]
 1324  1326 ?        Sl     0:00 gdm-session-worker [pam/gdm-launch-environment]
```
可以看到一个进程，对应了好几个**LWP**，即**轻量级线程**，可以把它理解成Linux下的线程。同样可以看到：**进程是个承载线程的容器**。

#### 线程的创建
创建函数：
```cpp
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);
//Compile and link with -pthread.
```
thread是个输入指针，间接赋值；attr是指定线程属性，NULL表示默认，将在之后用到；

start_routine就是个函数指针，也就是线程本身(一个**并列运行的函数**)；arg就是函数的参数；由于参数和返回值都是`void *`, 因此如果想传入或返回多个值，就用**结构体**。

成功返回0，不成功返回errno，注意是**返回**而**不是设置**，因此也**不能用**perror("func()")来报错。

编写makefile时加上`CFLAGS += pthread`和`LDFLAGS += pthread`
创建例子如下：
```cpp
void *myfunc(void *p)
{
	puts("thread is working!");
	return NULL;
}

int main()
{
	pthread_t thread_id;
	int err;
	puts("Begin!");
	err = pthread_create(&thread_id, NULL, myfunc, NULL);
	puts("End!");
	if(err)
		fprintf(stderr, "pthread err %s", strerror(err));
	exit(0);
}
```
最后却只输出了`Begin! End!`，没有输出`thread is working`，这是因为**线程的调度取决于调度器的调度策略**，**main线程**中创建了个线程，但是这个线程还没有被调度，main线程中就exit(0)**将进程结束**掉了(进程的正常终止)。

* 线程的终止
线程终止有三种方式：
1).线程从启动例程返回，返回值就是线程的退出码；若最后一个线程从其启动例程返回则进程**正常终止**
2).线程可以被**同一进程中的其他线程**取消；最后一个线程对其取消请求作出响应则进程**异常终止**
3).线程调用`pthread_exit()`函数，相当于进程的`exit()`；若最后一个线程调用`pthread_exit()`函数则进程**正常终止**

函数`pthread_join()`相当于**进程中的wait()**，完成线程**收尸**。
```cpp
void pthread_exit(void *retval); //结束线程并且通过retval返回值
int pthread_join(pthread_t thread, void **retval); //等待线程thread终止并获取其返回值到retval，retval也是个间接赋值
```
这两个函数是**配套使用**的，线程A函数中通过`pthread_exit()`返回值，在**同一进程**中使用`pthread_join`等待A的那个线程就可以通过`retval`获取其返回值。
那么:
```cpp
void *myfunc(void *p)
{
	puts("thread is working!");
	void *ret;
	pthread_exit(ret);
}

int main()
{
	pthread_t thread_id;
	int err;
	void *ret = NULL; //返回值
	puts("Begin!");
	err = pthread_create(&thread_id, NULL, myfunc, NULL);
	pthread_join(thread_id, &ret); //ret主调函数分配内存
	puts("End!");
	if(err)
		fprintf(stderr, "pthread err %s", strerror(err));
	exit(0);
}
```
main线程创建了thread_id线程，并且pthread_join**等待这个线程运行完给它收尸**，然后再执行后续代码。因此输出一定是：`Begin! thread is working! End!`
* 栈的清理
两个函数：
```cpp
void pthread_cleanup_push(void (*routine)(void *), void *arg); //和atexit差不多，挂上钩子函数
void pthread_cleanup_pop(int execute);
```
类似于进程中的钩子函数`atexit()`，但是钩子函数只能自己`atexit`挂钩子，自己不能左右取下来执行的过程。

而`pthread_cleanup_pop(int execute)`，取下来一个钩子函数可以设定`execute`，如果**为1则执行**该钩子，**为0不执行**。

更重要的，这两个函数必须**成对出现**，因为这两个函数是**宏展开**，成队出现才有匹配的大括号，否则会出语法错误。

* 线程的取消选项
```cpp
int pthread_cancel(pthread_t thread);
```
比如用两个线程寻找二叉树的两个子树里的一个确定结点，如果一个线程找到的话那么另一个线程就不必做了，直接取消那个线程。

本函数就是对线程thread发出**取消请求**，该线程响应取消后，可被pthread_join函数收尸。

取消的状态有两种：响应和不响应。
```cpp
int pthread_setcancelstate(int state, int *oldstate);
PTHREAD_CANCEL_ENABLE //state值表示响应
PTHREAD_CANCEL_DISABLE //不响应
```
设置是否响应取消，不响应的话会**忽略**收到的取消请求。

设置响应方式
```cpp
int pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void); //什么都不做，就是一个cancle点，用于推迟取消
PTHREAD_CANCEL_DEFERRED //推迟取消，收到取消请求后直到到达cancle点才响应
PTHREAD_CANCEL_ASYNCHRONOUS //异步取消，收到请求立刻响应取消
```
响应取消分为：异步取消和推迟取消(默认，推迟至cancle点再响应，cancle点都是可能引发阻塞的系统调用)
```cpp
fd1 = open();
if(fd1 < 0)
	exit(1);
pthread_cleanup_push(); //释放fd1的操作
fd2 = open();
```
推迟取消：如果在执行挂钩函数之前收到了**取消请求**，不会立刻响应，导致fd1没法释放，而是等到**cancle点**即打开fd2处**才响应**取消（open是可能阻塞的系统调用），这时候钩子函数已经成功挂上了，如果失败可以释放fd1.

* 线程分离
```cpp
int pthread_detach(pthread_t thread);
```
让thread线程分离出去**自生自灭**，之前线程资源的回收是需要**另一个**线程`pthread_join`，而分离出去的话线程资源由**系统自动回收**。

#### 线程竞争实例
多个线程之间可能涉及资源竞争。这部分的例子可以看之前OS作业的内容。

创建多个线程`pthread_creaate()`使用同一个函数的话，比如归并排序就是，这两个线程函数体的**代码段共用一份**，但是两个线程的**栈是独立的**。

如果多个线程不加控制地访问同一个共享资源，那么程序行为就无法控制了，接下来将介绍**互斥量**来解决这个问题，访问前加上锁，访问后解锁。

#### 线程同步
* 互斥量
**切记！！！**：互斥量**锁住的是代码**而不是变量。即锁住的是访问共享资源的代码，也就是**临界区**。

互斥量和信号量的关系：互斥量是信号量的特殊情况及**资源数只有1**，互斥量保证资源**独占**，而信号量的值可以>1，即可以有n个资源。

相关函数：
```cpp
pthread_mutex_init()
pthread_mutex_lock()
pthread_mutex_unlock()
pthread_mutex_destroy()
```
这部分的实现也见OS作业。互斥量其实完成的是对**临界区**的**单独访问**，临界区就是一个访问共用资源的**程序片段**，注意是程序片段，我们上锁的对象不是变量，而是**访问变量的代码**。

比如来个例子，定义4个线程分别打印`a b c d`，要求程序严格按照`abcd abcd abcd`一组一组打印。这个就是个典型的同步问题，显然我们需要三个信号量(当然这里用互斥量OK)，每个线程使用一个锁，打开下一个线程的锁，好比PV操作。

```cpp
pthread_mutex_t mut[4];
void *thr_func(void *p)
{
	int n = (int)p;
	int c = 'a' + n;
	while(1)
	{
		pthread_mutex_lock(mut + n); //相当于P操作，阻塞在这里等着自己可以拿到锁
		write(1, &c, 1);
		pthread_mutex_unlock(mut + (n+1)%4); //解锁下一个进程需要的锁，相当于V操作
	}
	pthread_exit(NULL);
}
int main()
{
	int i, err;
	pthread_t tid[4];
	for(i = 0; i< 4; i++) //创建四个线程和四把锁并且都锁上
	{
		pthread_mutex_init(mut + i, NULL);
		pthread_mutex_lock(mut + i);
		err = pthread_create(tid+i, NULL, thr_func, (void *)i); //给线程函数thr_func传入参数i,强转成void *
		if(err)
		{
			fprintf(stderr, "pthread_create error: %s", strerror(err));
			exit(1);
		}
	}
	alarm(3); //3秒后终止程序
	pthread_mutex_unlock(mut+0); //打开第一把锁，好让第一个线程可以P，好比多米诺骨牌的第一个
	for(i = 0;i<4;i++)
	{
		pthread_join(tid[i], NULL);
	}
	exit(0);
}
```
别忘了makefile里面加上`CFLAGS += -pthread; LDFLAGS += -pthred`;

#### 线程池的实现
这里实现的是个任务池，上游的main线程不断把任务放到池中，下游的三个线程抢任务。同样实现求质数，三个线程抢任务。

三个线程抢任务其实就是P操作，加锁再抢，因此需要一个互斥量。而main线程负责发任务，这四个线程对一个任务操作，也就是**全局变量**num

其实就是信号量PV操作的pthread实现，伪码如下：

三个线程中：
```cpp
while(1)
{
	pthread_mutex_lock(); //P信号量
	while(num == 0) //发现还没任务
	{
		pthread_mutex_unlock(); //松一下锁
		sched_yield(); //出让调度器给别的线程，并且不影响颠簸
		pthread_mutex_lock();
	}
	if(num == -1) //没任务了，跳出循环结束
	{
		pthread_mutex_unlock(); //!!!!!!!!!!!!临界区中的跳转如果跳到临界区外必须先解锁再跳！！！！！！
		break;
	}
	i = num; //取i
	num = 0;
	pthread_mutex_unlock();//V信号量
	//对i执行计算操作
}
```
main线程中：
```cpp
for(i = LEFT; i<RIGHT;i++)
{
	pthread_mutex_lock(); //要放任务时先P
	while(num != 0) //发现任务还没被取走
	{
		pthread_mutex_unlock(); //松一下锁
		sched_yield(); //出让调度器给别的线程，并且不影响颠簸
		pthread_mutex_lock();
	}
	num = i; //放任务
	pthread_mutex_unlock(); //放好任务再V
}

pthread_mutex_lock()
while(num != 0)
{
	pthread_mutex_unlock(); //松一下锁
	sched_yield(); //出让调度器给别的线程，并且不影响颠簸
	pthread_mutex_lock();
}
num = -1; //最后一个任务被拿走后放-1
pthread_mutex_unlock()
```
但是这个程序还是有很多**忙等**的地方，因为信号量的P操作是阻塞的，而且很有可能该P到信号量的线程P不到，不该P到的线程却经常P到，全看命。

虽然完成了同步，但是效率很低很盲目，因为信号量这个**抢锁机制**本身就是盲目的，很有可能你抢到了锁，但是你**并不需要这个锁**，需要这个锁的人却没有抢到。我们只是用**固定数量**的锁这个信息对多个线程之间执行逻辑的一种**粗暴的管理方式**，虽然逻辑是正确的。这就是**查询法**的弊端了，如果改成**通知法**，那就会好很多，也就是之后的**条件变量**。

#### 条件变量

条件变量是和互斥锁**一起使用**的，条件变量之所以要和互斥锁一起使用，主要是因为互斥锁的一个明显的特点就是它只有两种状态：锁定和非锁定，而条件变量可以通过允许**线程阻塞**和等待另一个线程**发送信号**来**弥补**互斥锁的不足。条件变量不是锁，但是也可以造成线程阻塞，并且用的是**通知法**，可以减少不必要的**竞争**。

如果仅仅是mutex，那么，不管共享资源里有没数据，**生产者及所有消费者**都全**一窝蜂**的去抢锁，会造成资源的浪费。有了条件变量机制以后，只有生产者**完成生产**，才会引起消费者之间的竞争，提高了程序效率。




```cpp
int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr); //初始化条件变量cond并设置属性attr
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; //静态初始化方法
int pthread_cond_destroy(pthread_cond_t *cond); //销毁条件变量
int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex); //阻塞等待一个条件变量
int pthread_cond_signal(pthread_cond_t *cond); // 唤醒至少一个阻塞在条件变量上的线程
int pthread_cond_broadcast(pthread_cond_t *cond); // 唤醒全部阻塞在条件变量上的线程
```
pthread_cond_wait函数的作用如下：
阻塞等待一个条件变量。具体而言有以下三个作用：

* 阻塞等待条件变量cond（参1）满足；
* 释放已掌握的互斥锁mutex（解锁互斥量）相当于pthread_mutex_unlock(&mutex);
* 当被唤醒，pthread_cond_wait函数返回时，解除阻塞并**重新抢互斥锁**
* 其中1、2.两步为一个**原子操作**。
可以看到区别，之前全用mutex实现的话，抢到锁之后如果条件不满足，只是稍微松一下锁然后继续加锁：
```cpp
pthread_mutex_lock()
while(num != 0)
{
	pthread_mutex_unlock(); //松一下锁
	sched_yield(); //出让调度器给别的线程，并且不影响颠簸
	pthread_mutex_lock();
}
```
而使用信号量就阻塞wait在这里**等待通知**，不再盲目抢锁，被通知后才抢，即：
```cpp
pthread_mutex_lock(&mutex);
while (head == NULL) {      //如果条件不满足，就没有抢锁的必要，一直阻塞等待
    pthread_cond_wait(&has_product, &mutex);
}
```
生产者消费者的例子：
```cpp
typedef struct msg {
    struct msg *next;
    int num;
}msg_t;

msg_t *head = NULL;
msg_t *mp = NULL;

/* 静态初始化 一个条件变量 和 一个互斥量*/
pthread_cond_t has_product = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex =PTHREAD_MUTEX_INITIALIZER;

void *th_producer(void *arg)
{
    while (1) {
        mp = malloc(sizeof(msg_t));
        mp->num = rand() % 1000;        //模拟生产一个产品
        printf("--- produce: %d --------\n", mp->num);

        pthread_mutex_lock(&mutex);
        mp->next = head;
        head = mp;
        pthread_mutex_unlock(&mutex);

        pthread_cond_signal(&has_product);      //唤醒线程去消费产品
        sleep(rand() % 5);
    }
    return NULL;
}

void *th_consumer(void *arg)
{
    while (1) {
        pthread_mutex_lock(&mutex);
        while (head == NULL) {      //如果链表里没有产品，就没有抢锁的必要，一直阻塞等待
            pthread_cond_wait(&has_product, &mutex);
        }
        mp = head;
        head = mp->next;        //模拟消费掉一个产品
        pthread_mutex_unlock(&mutex);

        printf("========= consume: %d ======\n", mp->num);
        free(mp);
        mp = NULL;
        sleep(rand() % 5);
    }
    return NULL;
}

int main()
{
    pthread_t pid, cid;
    srand(time(NULL));

    pthread_create(&pid, NULL, th_producer, NULL);
    pthread_create(&cid, NULL, th_consumer, NULL);

    pthread_join(pid, NULL);
    pthread_join(cid, NULL);
    return 0;
}
```
#### 读写锁
读锁：相当于共享锁；写锁：相当于互斥锁
#### 线程属性
pthread_arrt_t类型，可以在创建线程时设置各种属性，比如线程栈大小等等

```cpp
Thread attributes:
   Detach state        //分离状态，被pthread_join回收或者被系统回收
   Scope               //作用域
   Inherit scheduler   
   Scheduling policy   //调度策略
   Scheduling priority //调度优先级
   Guard size          //线程栈保护区大小
   Stack address       //线程栈地址
   Stack size          //线程栈大小
```
创建时设置attr参数即可:
```cpp
pthread_attr_t attr;
pthread_t tid;

pthread_attr_init(&attr);
pthread_attr_setstacksize(&attr, 1024*1024);
err = pthread_create(&tid, &attr, func, NULL); //创建线程时传入attr
pthread_attr_destroy(&attr);
```
* 线程同步的属性：
1.互斥量属性：
	
#### 重入
多线程相比信号更容易出现重入问题，比如多个线程运行同一个函数。

我们现在用的所有的**标准IO函数**都支持多线程并发，虽然同一个进程下的线程**共用**一份IO资源，但是所有标准IO函数都是**先锁住缓冲区**再操作后unlock。

有对应的线程不安全版本`_unlocked`，即不锁缓冲区：
```cpp
int getc_unlocked(FILE *stream);
int getchar_unlocked(void);
int putc_unlocked(int c, FILE *stream);
int putchar_unlocked(int c);
int fgetc_unlocked(FILE *stream);
int fputc_unlocked(int c, FILE *stream);
```
* 线程和信号的关系
线程和信号是两种不同的机制，注意信号是软件层面的中断，我们之前讲的是**进程**之间的信号；而线程中涉及到的信号量其实是**资源量**，是不同的概念。

一个进程里有多个线程，**每个线程**有自己的mask位图和pending位图；而进程没有mask位图**只有**pending位图；进程间发信号体现在进程的pending位图上，线程间发信号则体现在线程pending位图上；我们知道CPU**调度的单位是线程**，线程从内核态返回时用线程的mask和进程的pending按位与，再和自己的pending按位与，之后再进行对应信号处理函数。

相关函数：
```cpp
int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset);
//好比进程中的sigprocmask(2)，改变线程的信号屏蔽字
int pthread_kill(pthread_t thread, int sig);
//给别的线程发信号
```

* 线程和fork的关系

fork好比memcpy，创建了一个独立的进程，即使有写时拷贝技术，它们的C的虚拟空间本质上还是独立的，因此代价很高。

而线程就是**当前进程空间内**扔出去的**一个函数**，因此创建更快更方便。而且线程栈独立但是共享进程的代码段和全局变量等等资源，通信起来更方便。

那就有个问题：如果一个进程里有多个线程，那么fork之后，子进程里有几个线程？这个不同标准的fork实现也不同，POSIX线程原语的实现是子进程中只包含**调用fork**的那**一个线程**；DCE源于就是复制所有的线程。

#### 线程模式
* 流水线模式，这个模式线程的强大功能体现地不是很明显，因为还是一个接着一个执行
* 工作组模式，把一个大任务分成几个小任务，最后汇总或者分别输出，好像C/S模型
