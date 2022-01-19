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
	* 栈的上面就是重量级的**main函数参数和环境变量**了，这部分是在**用户区的最上方**。从这里就清楚了之前`setenv()`和`getenv()`函数的真面目了，环境变量本身就在我**自己的**用户空间中，set和get都是自己家事。更细的，如果set修改进程的环境变量，字符串太长的话，是在**堆区**开辟内存，用env[i]指针指向那个内存。

![image](https://user-images.githubusercontent.com/55400137/150061947-1037cfc6-b25f-43a2-b0c6-5f23f7a1b907.png)

更准确详细的内存布局：
![image](https://user-images.githubusercontent.com/55400137/150064134-9f00848c-1a1a-4ffb-a53c-1ba5406fbc5b.png)

**注意**：在.data段中还可以分为RO data只读数据段和RW data可读写数阶段，只读数据段包括全局const常量和字符串常量。

* **内核区**
这部分有个文章很清楚:[从环境变量到整个Linux进程运行](https://www.zhyingkun.com/markdown/environ/)

通过`ps`命令查看当前进程快照，`ps asf`命令查看所有进程的详细信息以及关系。**ps即`process snapshot`进程快照**
然后可以用`pmap [pid]`命令查看进程号为pid的进程的**内存图**，**pmap即`process memory map`进程内存图**
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
#### fork之后父子进程的区别
上面提到了就是全员复制创建新进程，连执行位置都一样。但是还是有一些不一样的地方：
* 返回值不一样：
* 父子进程的pid和ppid不同
* 未决信号和文件锁不继承：未决信号即还没来得及响应的信号
* 资源利用量清零
#### init进程
pid = 1，是所有进程的祖先进程，
不得不提到那个快被废弃的函数`vfork()`

### 进程的消亡及资源的释放
### `exec()`函数族
### 用户权限及组权限
### 扩展：解释器文件
### `system()`函数
### 进程会计
### 进程时间
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
