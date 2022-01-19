> Linux系统C语言编程，继续胡言乱语中。。。
# 进程
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

* **内核区**
这部分有个文章很清楚:[从环境变量到整个Linux进程运行](https://www.zhyingkun.com/markdown/environ/)
### 关于库
### 函数的跳转
### 进程资源的获取和控制

## 进程本身
## 并发
## 信号
多进程并发(信号量)
多线程并发
## 线程
## 数据中继
## 高级IO
## IPC(进程间通信)
