> Linux下C语言系统编程

## I/O
### 基本概念
sysio系统调用I/O(文件I/O) 和 stdio标准I/O
> 这部分内容基本靠man走天下了，手册给出函数接口，一看包含头文件；二看参数和返回值，顶多就是个指针，辨析一下输入输出；三看描述，基本就会用了
* 系统调用IO**直接**和系统kernel对话，在不同的系统下kernel是不一样的，这时候标准IO的概念就应运而生，标准IO是依赖于系统调用IO来实现的，**屏蔽了和kernel的对话**，相当于抛给程序员一个**接口**，因此**移植性更好**。
比如打开文件，C语言的标准IO提供的是`fopen()`，这个标准IO在Linux环境下依赖的系统IO是`open()`，而在windows环境下依赖的是`openfile()`，不同的系统下C**标准库有不同的实现**，因此如果需要移植性优先使用标准IO。
* C语言的**标准IO函数：FILE类型贯穿始终**，是一个**struct结构体**的typedef。直接`man fopen`即可查看对应手册

### 标准IO库函数
`fopen();`
* 各种模式: r只读 w写 a附加写，+读写，b二进制方式
* linux环境下的b是可以忽略的，因为linux不区分文本文件和二进制文件，只有一个stream的概念
* 出错后会自动将错误码填写到errno中，对应数字的错误可以在/usr文件夹中查看，可以用perror函数，或者用strerror函数查看对应errno的错误描述，errno在<errno.h>头文件中
* fopen()返回的FILE* 指针指向的内存究竟指向哪里？显然不能在栈区，不然这就是返回一个局部变量的指针了；然后不能在静态区，不然这个函数只会分配一次内存，打开一个文件是可以，多个文件就会冲掉内存里的东西；只能是堆区了。这里就可以看出fclose()的作用了，就是释放堆区分配的内存。
  * 如果一套API返回一个指针，并且有对应的close销毁函数，那么可以确定这内存一定分配在堆区。
* 资源的上限：一个进程可以打开文件的最多个数
```cpp
int main()
{
  FILE* fp = NULL;
  int count = 0;
  while(1)
  {
    fp = fopen("tmp", "w");
    if(fp == NULL)
    {
      fprintf(stderr, "open max count is %d\n", count);
      exit(0);
    }
    count++;
  }
}
```
* 输出1021，因为每个进程一创建默认打开三个stream文件，即著名的`stdin, stdout, stderr`。而我们fopen是标准IO也是打开流，因此一共是**最多打开1024个文件**。
* 可以通过`ulimit -a`来显示这些限制
* **文件权限**
  * 可以通过`ll`命令查看，d-rwx-rwx-rwx，d代表是否为目录，后面三个即不同用户组的读、写、执行。三个**八进制数**，全权限为`777`。
  * 普通文件的设置方式是通过`~umask & 0666`，`umask`直接命令行输入就会显示，一般为`0022`或者`0002`。因为要取反，因此`umask`的值越大，文件的权限越少。

`fputc();`
`fgetc();`
* 看man手册会发现，fgetc == getc == getchar(stdin)，fgetc和getc的函数原型都是`int ..(FILE* stream)`，而getchar只是将参数设置成`stdin`而已。
* putc(c) == fputc(c) == putchar(c, stdout)，和上一条类似， 其实`fprintf(stderr, ....)`也类似，`printf`只是默认输出流为`stdout`。
* 看man用到函数时一定要把相应的**头文件**包含进来，比如`stdlib.h`不包含也能运行但是会警告，但是隐患很多。
```cpp
//复制文件的简单实现
int main(int argc, char** argv)
{
	if(argc < 3)
	{
		fprintf(stderr, "Usage error: %s to fewer arguments!\n", argv[0]);
		exit(1);
	}
	FILE *fpsource = fopen(argv[1], "r");
	FILE *fpdest = fopen(argv[2], "w");
	if(fpsource == NULL || fpdest == NULL)
	{
		fprintf(stdout, "open file error%d: %s\n", errno, strerror(errno));
		exit(0);
	}
	char c = 0;
	while((c = fgetc(fpsource))!=EOF)
	{
		fputc(c, fpdest);
	}
	fclose(fpdest);
	fclose(fpsource);
	exit(1);
}
```
`fputs();fgets();`
* fgets在man手册里直接说明非常危险了，要用的话直接用`fgets(char *s, int size, FILE *stream)`，也就是`fgets(.., .., stdin);`从标准输入读取。fgets会在读取size-1个字符或者读到'\n'的时候停止，在尾部加上一个`\0`。每个文件末尾都**默认有个换行符**，即使看不到，因此fgets一定可以正常停止。
* `fputs(const char *s, FILE *stream)`;可以将字符串输入到任意文件中

`fread();fwrite();`
* 二进制流的读写操作，当然Linux环境下不区分二进制和文本文件的。
```cpp
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
```
返回成功读的item的个数。一般都将size尽可能小点，比如一个小单元，而读的次数多点。使用的时候一定要**根据返回值**成功读到几个对象就写几个对象：
```cpp
while((n = fread(buf, 1, 1024, fp)) > 0)
{
  fwrite(buf, 1, n, fp1); //这里是成功读到的次数，因为最后一次可能不满1024！
}
```
`fseek();ftell();rewind();`
* 用来操作**文件位置指针**
```cpp
 int fseek(FILE *stream, long offset, int whence); //将文件位置指针移动到whence再偏移offset的位置，whence可以取SEEK_SET, SEEK_CUR,SEEK_END，成功返回0，失败返回-1并且设置erron
 long ftell(FILE *stream); //返回当前文件位置指针的字节数
 void rewind(FILE *stream); //将流fp的文件位置指针设置到文件开始处，相当于fseek(fp, 0L, SEEK_SET); 0L表示为long类型
```
* feek和ftell还有个问题，由于偏移位置是long类型(32bit)，那么文件大小超过2GB就会出现负数。。因为补码首位是负权，因为这个函数太古老了，但是也遵循C99
`fflush(FILE* stream);`
* 刷新输出缓冲区，输出缓冲区是碰见换行符'\n'才刷新输出的，因此下面的程序：
```cpp
int main()
{
  printf("before while");
  while(1) { ;} 
  printf("after while");
}
```
你以为会输出一句`before while`，但是实际上**什么都不输出**。因为输出缓冲区没有碰见回车所以不刷新，也就没有输出。
而下面这样:
```cpp
 printf("before while\n");
 while(1) { ;} 
 printf("after while");
```
就可以输出第一句，因此printf最好**在结尾处都加上\n**
* 而fflush的作用就是**强制刷新输出流的缓冲区**，如果fp==NULL，那就刷新**所有**打开的输出流，如：
```cpp
 printf("before while");
 fflush(stdout);
 while(1) { ;} 
 printf("after while");
```
`printf();scanf();`
* 重要的是fprintf和sprintf对应文件IO和串IO，fprintf输出到一个流(FILE*)，sprintf输出到一个内存块。通过sprintf可以实现把数字转换成字符串的功能，类似于atoi函数的逆功能。
* scanf的功能等同于fscanf(stdin, "form..", &a, ....);，第一个也是获取的流
> 缓冲区的作用：大多数情况下是好事，用来**合并系统调用**，有几种刷新方式：
> * 行缓冲：换行的时候刷新，满了的是时候刷新，强制刷新（**stdout**是这样，因为是终端设备
> * 全缓冲：满了的是时候刷新，强制刷新（**默认情况**的刷新方式，只要不是终端设备都是全缓冲
> * 无缓冲：比如**stderr**，需要立即输出的内容，根本**不缓冲直接输出**。设置缓冲区刷新方式的函数为:setvbuf()，模式就是前面说的三种 _IONBF unbuffered _IOLBF，line buffered，_IOFBF fully buffered

`getline();`
如何完整读取一行字符串？gets本身就有缺陷，fgets虽然设置了size参数，只能读size-1个字符避免了溢出问题但这还不是我们想要的效果，我们想要输入多长就读多长，而不用事先确定好所谓的size那么麻烦。显然就是**动态分配内存**了
```cpp
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
```
这个API非常经典，lineptr二级指针显然是个**输出指针**，在函数内部动态分配内存，也就是我们需要把一个`char *`类型的地址传给它；n也一样需要把一个`size_t`类型的地址传给他，在函数内部完成**间接赋值**，指针的意义也就体现出来了。
但是C并没有提供释放这部分内存的API，只能说这是个可控的**内存泄漏**。

### 系统调用IO
* 系统调用IO操作：`open, close, read, write, lseek`
* 之前介绍的所有标准IO库函数都是**基于**上面这几个系统调用IO实现的

#### 文件描述符实现原理
FILE类型结构体，里面存放着文件描述符和偏移位置，**缓冲区**等等，而FILE结构体根据之前的分析我们知道存放在**进程内存空间的堆区**。
* 每个进程的PCB中都有**自己的**文件描述符表，文件描述符是文件描述符表的**数组下标**，是一个**数字**。之前`ulimit -a`显示出的进程可以打开最大资源数的那个1024，要清楚**概念的不同**：
  * 文件描述符fd：0， 1， 2，优先使用当前**允许的最小数字**，也就是说如果打开了0 1 2 3 4 5，现在关闭3，下一个使用的不是6而是3.
  * stream流：stdin, stdout, stderr
* 对于文件描述符-系统打开文件表-inode表的关系：[文件描述符-系统打开文件表-inode表](https://www.jianshu.com/p/ad879061edb2)
  * 需要知道的是，文件描述符表是存放在进程控制块中的，是个**非常简单**的表，大小一般1024个表示进程能存放文件描述符的最大值，每个单元存放文件描述符(**数字**)以及对应的系统打开表的结构体**指针**
  * 系统打开文件表是Linux系统维护的，每个单元是个结构体，存放着当前偏移位置，以及文件描述符表中指向该结构体的**计数**。
    * **可以**设置文件描述符表让不同的项指针指向系统打开表中的**同一个**结构体，但是一般而言，每open**一次**文件，就在系统打开表中新生成一个结构体，进程的文件描述符表也新生成个描述符，一一关联起来。
    > 联系实际经验，一个文件当然可以fopen多次，而且这几个描述符都是互相独立的，对应系统打开表中的结构体也独立。每fopen**一次**在内部调用系统的**open函数**返回一个文件描述符(int)，在**堆区**分配个FILE并得到其指针，文件描述符表的指针指向系统打开表中**新创建**的一个结构体，结构体再指向inode。
    * 系统打开表中的结构有个被指向的计数，每删除一个指向自己的描述符表项计数--，只有计数==0时结构体才能释放，否则描述符表项中的指针就变成野指针了。。。
  * 文件inode表才真正涉及到文件，linux系统中每个文件都有个inode号，表中存着文件在**磁盘的位置**，**读写权限**等等。

#### 系统IO函数
`open();close()`
通过man可以知道所需头文件：
```cpp
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
// open(), openat(), and creat() return the new file descriptor， or -1  if
//       an error occurred (in which case, errno is set appropriately).
```
* 可以看到open返回一个文件描述符，也就是一个 **int**，下面是flags和前文提到的标准IO函数fopen的mode的**映射**：
  * r:O_RDONLY,  r+:O_RDWR, w:O_WRONLY|O_CREAT|O_TRUNC, w+:O_RDWR|O_CREAT|O_TRUNC (O_RDONLY只写，O_CREAT没有则创建，O_TRUNC截断即清空，这些权限都是**按位或**
  * 这里的open**不是重载**，这是个变参，像printf一样接受**多少个参数**都OK，如果**重载的话参数个数应该是确定的**。如果实际到O_CREAT那么最后一定要给文件一个权限参数，比如0600，如下：
```cpp
int main(int argc, char **argv)
{
	int src = 0, dest = 0;
	int a = 0, pos = 0, ret = 0;
	char buf[SIZE];
	umask(0000); //暂时将umask设置成0000，之后0644就直接设置了
	if(argc < 3)
	{
		fprintf(stderr, "Usage error: to fewer arguments\n");
		exit(1);
	}
	src = open(argv[1], O_RDONLY);
	if(src == -1)
	{
		perror("open()");
		exit(1);
	}
	dest = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
	while(1)
	{
		a = read(src, buf, SIZE);
    pos = 0;
    while(a > 0) //读入a个字节必须都成功写入，因为很有可能被打断
    {
		  ret = write(dest, buf + pos, a);
		  pos += ret;
      a -= ret;
    }
	}
	close(dest);
	close(src);
  exit(0);
}
```
同样close()的函数原型如下:
```cpp
#include <unistd.h>
int close(int fd);
//close()  closes  a  file descriptor
```
用来关闭一个文件描述符，参数类型就是**int**。

##### 系统IO和标准IO的区别和联系
* 联系：标准IO是依赖系统IO实现的
* 区别：
  * 系统IO操作对象是文件描述符(整数)，而标准IO操作对象是stream流，即流指针所指向的流，默认打开三个流stdin,stdout,stderr
  * 系统IO无缓冲区，响应速度快，每次系统调用都**实打实**从用户态切换到内核态再返回完成一次系统调用；而标准IO的缓冲在之前已经提到，响应速度慢但是吞吐量高，因为**合并了系统调用**，即一次系统调用完成的工作更多，因为是**缓冲区积攒**了好久的。
```cpp
int main(int argc, char **argv)
{
	write(1, "before", 6);
	while(1) {;}
	write(1, "after", 5);
	exit(0);
}
```
因此这样的程序是可以直接输出before的，不像之前的printf是stdout行缓冲输出不来东西。
* 但是用户体验到的响应速度几乎没有什么区别，他们更在意吞吐量，因此还是最好使用标准IO
* **注意**：标准IO和系统调用IO不能混用：
  * 因为标准IO看到的FILE结构体里面，可以想到存有打开文件的文件描述符fd，就是那个int；还有文件位置指针pos。而系统打开文件表中的结构体，**也存有一个文件位置指针**，这两个位置指针的值一样吗？显然不一样，因为标准IO还有**缓冲区**：
    * 如果写文件，在FILE里的pos++了，但是系统打开文件表中结构体的pos并没有变化，只有**缓冲区**刷新的时候才**一次性修改**系统打开表中的pos。
    * 同样如果读文件的时候，系统打开表中的pos一般都是直接+=512个字节读到cache中，而FILE中的pos却是根据操作方式一点点移动
```cpp
int main()
{
	putchar('a');
	write(1, "b", 1);
	putchar('a');
  write(1, "b", 1);
	putchar('a');
  write(1, "b", 1);
	exit(0);
}
```
因此这样的混用代码最后输出的却是`bbbaaa`，因为系统调用直接就扎入内核实现了，而标准IO还要等缓冲区满。
* 用`strace`命令可以跟踪进程的系统调用，用`time`命令可以检测进程运行的用户态、内核态以及实际的时间

#### 文件共享
多个任务共同操作一个文件或者协同完成任务。
多进程，多线程。。

#### 原子操作
不可分割的操作单元，要么全做要么不做，用于解决竞争和冲突
**重定向**： `dup, dup2`
把输出到屏幕上的内容重定向stdout到其他文件，先把文件描述符为1的close掉，然后再open一个文件，因为默认使用当前可用最小的fd的位置，因此这个open一定返回1，也就是说这个文件描述符表项指向改变了。之后**输出到stdout的内容仍然按照下标为1的文件描述符来**，只不过这个描述符的指针已经指向我们的文件了，代码如下：
```cpp
int main(int argc, char **argv)
{
	int fd = 0;
	close(1);
	umask(0000);
	fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, 0644);

	printf("hhahahahha");
	exit(0);
}
```
而dup函数的作用是，**复制一份**文件描述符的表项（其实重点就是指向系统打开表的指针指向），**放到**当前可用最小的文件描述符的位置，于是上面的做法：
```cpp
int fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
close(1);
dup(fd);   //显然dup之后副本表项和原表项指向同一个打开表中的结构体，因为指针一样
close(fd); //把原来的就关了吧
```
* 并发时的bug：上面代码中的`close(1); dup(fd);`显然会出现问题，如果`close(1)`之后被打断，其他的进程open了一个文件就把1的位置给占了
* 因此需要把这两步操作**合并成一个原子操作**，`dup2(newfd, oldfd);`，即`dup(fd, 1);`。它的作用是让newfd直接使用oldfd的位置，省去close和dup。阅读man可以知道当new和old值相等时就直接返回
* 但是这还有个问题就是最后的close(fd)，因为dup2的话fd就是位置1了，把自己给关了。因此需要加一个判断`if(fd != 1)`

#### fcntl和ioctl
文件描述符管家函数和io设备管家函数。。
/dev/fd/**虚目录**，显示的当前进程的文件描述符信息，当前进程指谁调用它谁就是当前：
`ls -a /dev/fd`，这里显示的就是`ls`进程的文件描述符信息。

## 文件系统

## 并发
多进程并发(信号量)
多线程并发

## IPC(进程间通信)
