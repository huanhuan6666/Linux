> Linux下C语言系统编程，胡言乱语。。。还是菜（

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
* linux环境下的b是可以忽略的，因为linux不区分文本文件和二进制文件，只有一个文件流stream的概念
* 出错后会自动将错误码填写到errno中，对应数字的错误可以在/usr文件夹中查看，可以用perror函数，或者用strerror函数查看对应errno的错误描述，errno在<errno.h>头文件中
* fopen()返回的FILE* **文件流指针**指向的内存究竟指向哪里？显然不能在栈区，不然这就是返回一个局部变量的指针了；然后不能在静态区，不然这个函数只会分配一次内存，打开一个文件是可以，多个文件就会冲掉内存里的东西；只能是堆区了。这里就可以看出fclose()的作用了，就是释放堆区分配的内存。
  * 如果一套API返回一个指针，并且有对应的close销毁函数，那么可以确定这内存一定分配在堆区。
* 资源的上限：一个进程可以打开文件的最多个数
下图表可以看到，FILE文件流结构体里面包含一个文件描述符(int)，是进程的文件描述符表的索引。文件描述符是后续会讲到的系统调用open的返回值。
![image](https://user-images.githubusercontent.com/55400137/149782064-6a3e6bb4-3c71-48bb-b8ff-fae247e78c62.png)

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

* 输出1021，因为每个进程一创建默认打开三个stream文件，即著名的`stdin, stdout, stderr`，因此一共是**最多打开1024个文件**。
* 可以通过`ulimit -a`来显示这些限制
* **文件权限**
  * 可以通过`ll`命令查看，d-rwx-rwx-rwx，d代表是否为目录，后面三个即不同用户组的读、写、执行。三个**八进制数**，全权限为`777`。
  * 普通文件的设置方式是通过`~umask & 0666`，`umask`直接命令行输入就会显示，一般为`0022`或者`0002`。因为要取反，因此`umask`的值越大，文件的权限越少。

`fputc();fgetc();`

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

* gets在man手册里直接说明非常危险了，要用的话直接用`fgets(char *s, int size, FILE *stream)`，也就是`fgets(.., .., stdin);`从标准输入读取。fgets会在读取size-1个字符或者读到'\n'的时候停止，在尾部加上一个`\0`。每个文件末尾都**默认有个换行符**，即使看不到，因此fgets一定可以正常停止。
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
 int fseek(FILE *stream, long offset, int whence); //将文件位置指针移动到whence再偏移offset的位置，whence可以取SEEK_SET, SEEK_CUR,SEEK_END，成功返回0，失败返回-1并且设置errno
 long ftell(FILE *stream); //返回当前文件位置指针的字节数
 void rewind(FILE *stream); //将流fp的文件位置指针设置到文件开始处，相当于fseek(fp, 0L, SEEK_SET); 0L表示为long类型
```

* feek和ftell还有个问题，由于偏移位置是long类型(32bit)，那么文件大小超过2GB就会出现负数。。因为补码首位是负权，这确实是个**缺陷**，因为这个函数太古老了，但是遵循C99
  `fflush(FILE* stream);`
* 刷新输出缓冲区，标准输出缓冲区是碰见换行符'\n'才刷新输出的，因此下面的程序：

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

* 重要的是fprintf和sprintf对应文件IO和串IO，fprintf输出到一个文件流(FILE*)，sprintf输出到一个内存块。通过sprintf可以实现把数字转换成字符串的功能，类似于atoi函数的逆功能。
* scanf的功能等同于fscanf(stdin, "form..", &a, ....);，第一个也是获取的流

> 缓冲区的作用：大多数情况下是好事，用来**合并系统调用**，有几种刷新方式：
>
> * 行缓冲：换行的时候刷新，满了的是时候刷新，强制刷新（**stdout**是这样，因为是终端设备
> * 全缓冲：满了的是时候刷新，强制刷新（**默认情况**的刷新方式，只要不是终端设备都是全缓冲
> * 无缓冲：比如**stderr**，需要立即输出的内容，根本**不缓冲直接输出**。设置缓冲区刷新方式的函数为:setvbuf()，模式就是前面说的三种 _IONBF unbuffered _IOLBF，line buffered，_IOFBF fully buffered

`getline();`
如何完整读取一行字符串？gets本身就有缺陷，fgets虽然设置了size参数，只能读size-1个字符避免了溢出问题但这还不是我们想要的效果，我们想要输入多长就读多长，而不用事先确定好所谓的size那么麻烦。显然就是**动态分配内存**了

```cpp
size_t getline(char **lineptr, size_t *n, FILE *stream);
```

这个API非常经典，lineptr二级指针显然是个**输出指针**，在函数内部动态分配内存，也就是我们需要把一个`char *`类型的地址传给它；n也一样需要把一个`size_t`类型的地址传给他，在函数内部完成**间接赋值**，指针的意义也就体现出来了。
但是C并没有提供释放这部分内存的API，只能说这是个可控的**内存泄漏**。

### 系统调用IO

* 系统调用IO操作：`open, close, read, write, lseek`
* 之前介绍的所有标准IO库函数都是**基于**上面这几个系统调用IO实现的

#### 文件描述符实现原理

FILE文件流结构体，里面存放着文件描述符和偏移位置，**缓冲区**等等，而FILE结构体根据之前的分析我们知道存放在**进程内存空间的堆区**。

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

* 类ls命令的实现，贯穿这一部分的所有内容。包括选项：-a(显示隐藏文件) -i(显示文件inode号) -l(展示用户和组的名称) -n(like -l,展示用户和组的ID)
* 终端命令的基本格式: `cmd --长格式 -短格式 非选项的传参`。比如ls --all等同于ls -a，传参可以文件路径名。
* 如何创建或者删除一个文件名为"-a"的文件？touch -a会直接把-a解析成选项并报错**缺少文件操作数**，这时候可以用个--占位符把选项位置占过去:`touch -- -a`就可以了，或者把路径写下来`touch ./-a`。删除也同样`rm -- -a`或者`rm ./-a`.

### 目录和文件

* 获取文件的属性
  `stat, fstat, lstat`系统调用
  stat获取的**就是文件的inode信息**，只不过把文件inode信息放到一个stat结构体里的对应字段里了，通过inode的手册即可得知。

```cpp
int stat(const char *pathname, struct stat *statbuf); //对于符号链接文件获取所指向文件的属性
int fstat(int fd, struct stat *statbuf);
int lstat(const char *pathname, struct stat *statbuf); //对于符号链接文件获取符号链接文件本身的属性
```

这几个函数又是典型的输入指针捏，而且unix的函数命名是很有规范的，fstat的参数就是个文件描述符，stat就是个文件名，lstat和stat参数一样但是对于符号**链接文件**的处理不同。
stat结构体如下：

```cpp
struct stat {
	dev_t     st_dev;         /* ID of device containing file  包含该文件的设备号*/
	ino_t     st_ino;         /* Inode number  文件inode号*/
	mode_t    st_mode;        /* File type and mode 文件类型和操作权限信息，就是drwxr--r--的那个*/
	nlink_t   st_nlink;       /* Number of hard links 硬链接数*/
	uid_t     st_uid;         /* User ID of owner 用户id*/
	gid_t     st_gid;         /* Group ID of owner 组id*/
	dev_t     st_rdev;        /* Device ID (if special file) 如果文件确实是个设备的话，设备的ID号*/
	off_t     st_size;        /* Total size, in bytes 大小字节数*/
	blksize_t st_blksize;     /* Block size for filesystem I/O 文件系统规定的块大小*/
	blkcnt_t  st_blocks;      /* Number of 512B blocks allocated 此文件占用的块数*/

	/* Since Linux 2.6, the kernel supports nanosecond
	  precision for the following timestamp fields.
	  For the details before Linux 2.6, see NOTES. */

	struct timespec st_atim;  /* Time of last access */
	struct timespec st_mtim;  /* Time of last modification */
	struct timespec st_ctim;  /* Time of last status change */

   #define st_atime st_atim.tv_sec      /* Backward compatibility */
   #define st_mtime st_mtim.tv_sec
   #define st_ctime st_ctim.tv_sec
};
```

* 同样可以用stat命令来获取文件信息，和上面字段都可以对应上，stat命令就是用上面的stat()函数封装的。

```cpp
  文件：c.txt
  大小：46        	块：8          IO 块：4096   普通文件
设备：801h/2049d	Inode：400481      硬链接：1
权限：(0644/-rw-r--r--)  Uid：( 1000/   njucs)   Gid：( 1000/   njucs)
最近访问：2022-01-16 12:58:32.530000000 +0800
最近更改：2022-01-16 12:58:28.894000000 +0800
最近改动：2022-01-16 12:58:28.894000000 +0800
```

* 注意st_size文件大小，只是文件的一个**属性**而已，而具体在磁盘上占了多少空间看的是st_blsize和st_blokcs，上面就是8* 4K = 32KB了，但是文件的实际字节数只有46B. 
* **空洞文件**：

```cpp
int main(int argc, char **argv)
{
	int fd = 0;
	if(argc != 2)
	{
		fprintf(stderr, "Usage: %s<pathname>", argv[0]);
		exit(1);
	}
	umask(0000);
	fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, 0644);

	lseek(fd, 5LL*1024LL*1024LL*1024LL, SEEK_SET);
	write(fd, "", 1); //注意这句一定不能少，否则就彻底为空洞文件，st_size=0
	close(fd);
	exit(0);
}
```

`./big big.txt`获取其文件状态如下：

```
文件：big.txt
大小：5368709121	块：8          IO 块：4096   普通文件
```

也就是size为5G，但是存在磁盘上只占32K。如果不写最后的`write(fd, "", 1);`的话，那么最后size=0，块数也=0。lseek移动超出文件长度的话，中间全部用`\0`来填写。并且这部分内容是**不会写到磁盘上的**，因为全0也没啥好写的反而占用空间。因此要深刻理解unix环境下**文件的size属性和实际上占用磁盘大小的区别**。

> 以及man手册的example也摘一下吧，看看人家代码是怎么写的:
>
> ```cpp
> int main(int argc, char *argv[])
> {
> struct stat sb;
> if (argc != 2) {
> fprintf(stderr, "Usage: %s <pathname>\n", argv[0]); //不像下面的有errno可以用，输出到stderr比较好因为不用缓冲>
> exit(EXIT_FAILURE);
> }
> 
> if (lstat(argv[1], &sb) == -1) {
> perror("lstat"); //因为系统调用一般都设置了errno
> exit(EXIT_FAILURE);
> }
> exit(EXIT_SUCCESS);
> }
> ```

用stat系统调用实现一下获取文件长度

```cpp
off_t flen(const char *name)
{
	struct stat_t res;
	if(stat(name, &res) < 0)
	{
		perror("stat");
		exit(1);
	}
	return res.st_size;
}
int main(int argc, char *argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: ...");
		exit(1);
	}
	fprintf(stdout, "%lld", flen(argv[1]));
	exit(0);
}
```

### 文件访问权限

stat获取的是文件的inode信息放到stat结构体的对应字段上了，因此很多细节定义都在inode的手册里`man 7 inode`。

对于文件类型和访问权限对应stat结构体里的st_mode字段，给出了各种宏类型以及使用例：

```cpp
//Thus, to test for a regular file (for example), one could write:
stat(pathname, &sb);
if ((sb.st_mode & S_IFMT) == S_IFREG) {
/* Handle regular file */
}
```

以及更直接的检测类型的宏：

```cpp
S_ISREG(m)  is it a regular file?
S_ISDIR(m)  directory?
```

文件权限的更改

* `chmod`命令：`chmod u+x`表示给用户加上执行权限，类似的`g+r, o-w`表示给同组用户加上读，**删除**其他用户的写，`a+x`表示给所有用户加上执行权限，直接`chmod 666`表示权限设置成`rw-rw-rw-`。
* `chmod()`系统调用函数，同样也是在进程执行时临时改变某个文件的权限。也是经典的函数族：`int chmod(const char* path, mode_t mode); int fchmod(int fd, mode_t mode);`

umask相关

* `umask`命令可以在命令行显示当前umak指
* `umask()`系统调用函数，可以在进程**执行时**，**临时改变**umask的值

### 文件类型

**文件类型一共七种**：目录文件d，常规文件-，字符设备文件c，块设备文件b，符号链接文件l，socket文件s，管道文件p
更深入的**st_mode位模式**，

```cpp
	The following mask values are defined for the file type:

           S_IFMT     0170000   bit mask for the file type bit field //前四位代表文件类型

           S_IFSOCK   0140000   socket
           S_IFLNK    0120000   symbolic link
           S_IFREG    0100000   regular file
           S_IFBLK    0060000   block device
           S_IFDIR    0040000   directory
           S_IFCHR    0020000   character device
           S_IFIFO    0010000   FIFO
	
	The following mask values are defined for the file  mode  component  of
       the st_mode field:

           S_ISUID     04000   set-user-ID bit
           S_ISGID     02000   set-group-ID bit (see below)
           S_ISVTX     01000   sticky bit (see below)

           S_IRWXU     00700   owner has read, write, and execute permission
           S_IRUSR     00400   owner has read permission
           S_IWUSR     00200   owner has write permission
           S_IXUSR     00100   owner has execute permission

           S_IRWXG     00070   group has read, write, and execute permission
           S_IRGRP     00040   group has read permission
           S_IWGRP     00020   group has write permission
           S_IXGRP     00010   group has execute permission

           S_IRWXO     00007   others  (not  in group) have read, write, and
                               execute permission
           S_IROTH     00004   others have read permission
           S_IWOTH     00002   others have write permission
           S_IXOTH     00001   others have execute permission
```

这样就非常清楚了，比如`S_IFMT     0170000`，0开头说明**八进制数**，然后第一个1是单独1位，后面5*3=15位。前4位和文件类型相关，`S_IFMT`全1相当于mask；低12位就是权限相关：拥有者权限、同组用户权限、其他用户权限，并且都有对应的mask；中间3位**特殊位**：设置用户ID位，设置组ID位以及**粘位**。

* 粘住位
  如果对一个目录设置了粘住位，则只有对该目录具有写权限的用户在满足 下列之一的情况下，才能删除或更名该目录下的文件：拥有此文件；拥有此目录；是超级用户。
  目录/tmp和/var/spool/uucppublic是设置粘住位的典型候选者——任何用户都可在这两个目录中创建文件。任一用户（用户、组和其他）对这两个目录的权限通常都是读、写和执行。但是用户不应能删除或更名属于其他人的文件，为此在这两个目录的文件模式中都设置了粘住位。

### 经典文件系统:FAT,UFS

> 文件系统作用：文件或数据的存储和管理。

#### FAT/微软早期的文件系统

* FAT也叫文件分配表，本质：**静态存储的单链表**，静态存储也就是**数组实现**的，而且要链表的话那么结构体里面的next域放的是**下一个结构体的数组下标**。
* 系统机构如下，一般有两个FAT，FAT2是主FAT1的**备份**，DATA是数据区。FAT中有一个**簇**的概念，其实就是一个固定大小的存储单元存放在数据区。FAT表项号和簇号**一一对应**，表项中存放下一个簇号或者结束标志，根据簇号读数据区的内容。读文件时，首先在目录项找到文件**第一个簇**的簇号，之后就根据这个簇号在FAT中一直读，根据next号在对应的数据区读簇，直到FAT表中读到结束。

#### UFS/Unix早期的文件系统

* 每个磁盘可以有多个分区，每个分区可以有多个块组/柱面组。在一个块组中，就是我们所熟悉的内容：首先是一些描述性信息，然后就是inode**位图**，之后是数据块**位图**，之后是inode结点区，最后是数据块区。
* 数据块区里面存储真正的文件数据，每个块大小可能为4KB。
* 而inode结点区的**每一个inode结构体**里面包括：之前所讲的**stat函数里面包含的所有字段信息**，以及一些亚数据信息和其他无关信息，之后就是重中之重：
  * 一个**包含15个数据块指针的数组**，指向的就是数据块区。前12个是**直接数据块指针**：即指针指向的数据块就是文件的数据，也就是可以放12*4KB = 48KB的数据。
  * 后三个分别为一级二级三级的指针，一个数据块中存`4K/8=512`个指针，因此一级可容纳：`512*4KB`的文件，二级可容纳`512*512*4KB`的文件，三级就是：`512*512*512*4KB=2^39B=512GB`的文件。

 > 也就是一个inode的数据块指针所涉及的大小已经达到TB级别了，现实生活很少碰见一个文件这么大的，因此UFS系统是不担心文件过大的。

* inode结点包含了文件的各种信息，但是**唯独没有包含文件名**。因为**文件名保存在目录文件中**，目录文件的内容为目录项，每个目录项为**文件名和对应的inode号**。因此从文件名获取文件信息时，从**根目录区**开始读根目录内容，然后一项一项读直到得到对应的文件inode号，然后找到这个inode结点即可得到文件内容。
   	
*  UFS缺陷的思考以及inode位图和块位图的**重要意义**
  * 显然UFS是非常擅长管理大文件的。但是有个重要的问题：inode结点区和数据块区的数量关系是**一对多**的，而不像FAT那样FAT表中项和数据区的簇一一对应。这就导致：在一个块组里，如果小文件很多，那么inode结点区耗尽，但是数据块区还有很多空闲；类似的，如果只有几个特大文件，那么数据块区耗尽但是inode结点还有很多没用。
  *  要快速找到一个空闲的inode或者看看数据块还有剩余多少，**inode位图和数据块位图**的作用就显现出来了，全是01扫描起来非常快，并且也可以快速查看被占用的块数等等。

![image](https://user-images.githubusercontent.com/55400137/149781072-6b2fcc9e-45e6-4e15-b47e-f3b747b857e2.png)

### 硬链接和符号链接

#### 硬链接

* 是目录项的同义词，相当于**给文件起别名**
* 通过`ln file file_link`命令将`file_link`硬链接到`file`上，这两个文件的**inode号一样**。
* 其实就是在所在目录的目录文件中增加了一项`file_link ---> file的inode号`，并且修改inode结点的硬链接数目，inode一样了那不就完完全全**同一个文件**么
* 有点像多个指针指向同一块内存。如果删除一个硬链接文件的话也相当于**删除一条目录项**，并将inode结点中的硬链接数目--，当数目为0时删除inode结点并释放占用的数据块。	

#### 符号链接

通过`ln -s file.txt file_s.txt`命令创建符号链接。符号链接文件和原文件是两个不同的文件，**inode不同**，输出如下：

```cpp
  文件：file_s.txt -> file.txt
  大小：8         	块：0          IO 块：4096   符号链接
设备：801h/2049d	Inode：408369      硬链接：1
/********************************************************/
 文件：file.txt
  大小：56		块：8          IO 块：4096   普通文件
设备：801h/2049d	Inode：408379      硬链接：2
```

* 可以看到inode号不同，有趣的是符号链接文件的**大小==原文件名的大小**，`file.txt`大小为8，并且**不占磁盘块**，因为一般符号链接存的那个文件名都**放在inode结点里了**。	
* 可以看出区别，硬链接是**普通文件**，显示为`-rwx-r--r--`；符号链接的**文件类型**是**符号链接文件**，显示为`lrwxr--r--`，即第一个字母为`l`。

#### 对比

* 硬链接不能跨分区建立，因为不同分区的inode号可能会发生冲突，不能给目录建立硬链接；符号链接可以跨分区，也可以给目录建立。

#### 相关系统调用函数

* `man 2 link()`函数，描述非常直白`link, linkat - make a new name for a file`，就是给文件起别名；
* 对应的`unlink()`和`remove()`系统调用。

> * `unlink()`的描述如下:`unlink() deletes a name from the filesystem.  If that name was the last link to a file and no processes have the file open, the file is deleted and the space it was using is  made  avail‐able for reuse.
>
> * `remove()`内部调用了`unlink()`，描述：`remove()  deletes a name from the filesystem.  It calls unlink(2) for files, and rmdir(2) for directories`



### 其他命令和系统调用

* 时间相关命令utime

* 目录的创建和销毁

  mkdir创建，rmdir删除

* 更改当前工作目录

  pwd命令，封装了getcwd()系统调用

  cd命令，封装了chdir()系统调用，以及fchdir()系统调用，可以切换进程的工作目录。

  这就涉及到安全问题，chroot**假根技术**将让进程认为当前工作目录就是根目录，但是chdir命令可能改变工作目录到真正的根上，也就是**chroot穿越**，很容易造成安全隐患。

* 分析目录/读取目录内容
  **用golb()函数进行目录解析**
  `glob()`库函数，使用`man 7 glob`的shell命令一样的方式，对文件进行**通配符匹配**，函数原型如下：
  ```cpp
	int glob(const char *pattern, int flags,
                int (*errfunc) (const char *epath, int eerrno), //显然这是个函数指针，可以自定义回调
                glob_t *pglob);
	int globfree(glob_t *pglob); //显然glob_t结构体是分配在堆区的，是个输出指针
	typedef struct {
               size_t   gl_pathc;    /* Count of paths matched so far  */     //符合匹配的文件个数和文件名的数组
               char   **gl_pathv;    /* List of matched pathnames.  */        //这两项非常像main函数的argc和argv，字符串的个数和字符串指针
               size_t   gl_offs;     /* Slots to reserve in gl_pathv.  */
           } glob_t;
  ```
	> * 即用*表示匹配多个字符，用?匹配单个字符，
	> * 用[...]表示范围匹配**单个**字符，比如`[][!]`表示匹配`] [ !`这三个字符；`[A-Fa-f0-9]`表示`[AB..Fab..f012...9]`，注意第一个[之后不能紧跟!
	> * 否则就匹配方括号**之外**的**单个**字符，如`[!0-9]`表示不匹配数字
用glob函数匹配文件示例：匹配/etc目录下a开头的.conf文件：
```cpp
#define PATTERN "/etc/a*.conf"

int myfunc (const char *epath, int eerrno)  //自定义回调函数，把出错信息输出出来
{
	puts(epath);
	fprintf(stderr, "ERROR MSG: %s\n", strerror(eerrno));
	return eerrno;
}
int main()
{
	glob_t tmp;
	if(glob(PATTERN, 0, myfunc, &tmp) != 0)
	{
		perror("glob");
		exit(1);
	}
	int count = tmp.gl_pathc, i = 0;
	char **allpath = tmp.gl_pathv;
	for(i = 0; i<count;i++)
	{
		puts(allpath[i]);
	}
	globfree(&tmp);	
	exit(0);
}
```
解析整个目录就更简单了，只需要把PATTERN设置成`/etc/*`，即可解析etc目录下的所有内容，**不包括隐藏**，因为这只是个模式`*`而已。
  
  **直接用目录文件的操作函数进行解析**：
  `opendir(), fdopendir(), closedir(), readdir()`返回的是一个`DIR *`的目录流结构体，类似于`FILE *`文件流，还有其他相关的结构体看man手册吧，这里大致说一下`readdir()`函数：
```cpp
#include <dirent.h>
struct dirent *readdir(DIR *dirp);
struct dirent {
	ino_t          d_ino;       /* Inode number */
	off_t          d_off;       /* Not an offset; see below */
	unsigned short d_reclen;    /* Length of this record */
	unsigned char  d_type;      /* Type of file; not supported
				      by all filesystem types */
	char           d_name[256]; /* Null-terminated filename */
};
```
可以看到返回的`dirent`**目录项结构体**，最重要的就是`ino_t`和`d_name`即inode号和文件名。像读文件那样一条条readdir()输出dirent结构体里的d_name即可完成glob例子实现的效果。

**练习**：`du`命令可以获取文件或者目录所占块的个数，自己实现一个`mydu`，达到类似的效果，涉及到各种库函数的使用以及递归和简单的递归优化。见另一个文件夹mydu。

### 系统数据文件和信息
* `/etc/passwd`即查看系统的用户详细信息，包括用户ID，组ID等等。在Linux环境下存放在`/etc/passwd`目录下面，但是在其他系统中的实现不同，因此提供标准函数：
```cpp
struct passwd *getpwnam(const char *name); //根据用户名查询用户信息
struct passwd *getpwuid(uid_t uid); //根据uid查询用户信息
struct passwd {
	char   *pw_name;       /* username */
	char   *pw_passwd;     /* user password */
	uid_t   pw_uid;        /* user ID */
	gid_t   pw_gid;        /* group ID */
	char   *pw_gecos;      /* user information */
	char   *pw_dir;        /* home directory */
	char   *pw_shell;      /* shell program */
}; //用户信息结构体passwd
```
passwd结构体里的字段和`/etc/passwd`文件里记录的字段一一对应。一般来说`root`用户的`uid = 0`。下面是通过输入用户uid来查询用户名的例子，使用getpwuid标准函数：
```cpp
int main(int argc, char **argv)
{
	struct passwd *pass;
	if(argc < 2)
	{
		fprintf(stderr, "Usage...\n");
		exit(1);
	}
	pass = getpwuid(atoi(argv[1]));
	puts(pass->pw_name);
	exit(0);
}
```
* `/etc/group`查询组信息，和用户信息类似的标准函数:
```cpp
struct group *getgrnam(const char *name);
struct group *getgrgid(gid_t gid);
struct group {
       char   *gr_name;        /* group name */
       char   *gr_passwd;      /* group password */
       gid_t   gr_gid;         /* group ID */
       char  **gr_mem;         /* NULL-terminated array of pointers
				  to names of group members */
};
```
同样`root`用户的`gid = 0`，同样也是返回一个指针，即被调用函数分配内存。
* `/etc/shadow`
这里就需要区分`sudo`命令和`su`命令，以及普通用户和root用户的区别了。root用户是真正的超级用户，如果要真正切换到root用户需要使用`su`命令并且输入root用户的密码，一般是不知道的；也可以用`sudo -i`输入当前用户的命令，切换到root用户。
而平时常用的`sudo`命令需要输入的是**当前用户的密码**，而不是root用户的密码，能够暂时切换到root用户执行东西，但是有时间限制，Ubuntu默认为15分钟。
`/etc/shadow`就需要root权限才能查看，可以看到每个用户加密后的密码，比如123asd=.加密后就是下面中间一大串的东西
```cpp
njucs:$6$dJpuCzZM$IrhOK/T7zWwj87v0GXPgZFv/sg.9npEPRDKV5Njgp4ha/XxsaOdMvyVXUVxzXknDib3p4Glf/eFVNKwxolNyg1:18696:0:99999:7:::
```
第一对$之间的6表示的是加密方式，代表SHA-512。第二对$中间的内容是杂质串，然后将原串和杂质串一起进行6表示的加密算法得到后面的密文。
**获取shadow文件项的库函数**：
```cpp
struct spwd *getspnam(const char *name);
char *crypt(const char *key, const char *salt);
char *getpass(const char *prompt); //必须用root用户才能执行这个函数，否则会段错误
```
getspnam通过用户名获取`spwd`结构体，和之前的类似，结构体字段和shadow文件里的对应，获取的是**密文**。
crypt加密函数，key就是原串，salt就是**杂质串**，最后得到加密后的**密文**。salt的说明如下，也就是说salt必须遵守格式`$id$salt$encrypted`，他会自己截取前三个$之前的部分作为加密方式和杂质串。
getpass输入密码的时候不显示到屏幕上，向linux那样。上古函数，不建议使用obsolete。prompt是需要输出的提示。
```cpp
If salt is a character string starting with the characters "$id$"  fol‐
lowed by a string optionally terminated by "$", then the result has the
form:
      $id$salt$encrypted
```
* 时间戳
每一个文件都有三个时间属性，在之前的stat提到过。而ls显示的mtime就是最近一次修改时间
```cpp
struct timespec st_atim;  /* Time of last access */
struct timespec st_mtim;  /* Time of last modification */
struct timespec st_ctim;  /* Time of last status change */
```
time()系统调用，从系统得到时间戳`time_t`类型，是个大整数。还有其他相关：
```cpp
time_t time(time_t *tloc); //返回一个大整数，同时也间接修改参数的那个指针，用哪个都可以
struct tm *gmtime(const time_t *timep); //是tm结构体，有几个字段，把一个time_t类型的东西转换成tm结构体。
time_t mktime(struct tm* tm); //把一个tm结构体转换成time_t类型
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm); //format格式化字符串，和printf那一批函数异曲同工，%Y%M%D啥的。从tm结构体里取字段格式化后放到s里，s和max就是个内存打包。
```
gmtime返回的指针指向静态区，因此后面的调用会冲掉之前的结果。mktime函数的参数不是const，因此mktime可以自动检查tm结构体的溢出情况并且调整，比如13月35日这样的。
下面是一个典中典的例子：
```cpp
int main()
{
	int count = 0;
	FILE *fp = NULL;
	time_t t;
	struct tm *tm;
	char buf[1024] = {0};
	fp = fopen("file.txt", "a+"); //附加写，注意本例用的全是标准IO，这就涉及到缓冲区的问题
	while(fgets(buf, 1024, fp) != NULL) count++; //求前面有多少行
	while(1)
	{
		time(&t); //获取当前时间
		tm = gmtime(&t); //放到时间结构体里
		fprintf(fp, "%d  %d-%d-%d  %d:%d:%d\n", count++, 1900+tm->tm_year, 1+tm->tm_mon,....); //注意tm_year从1900年开始，tm_mon从0开始
		//fflush(fp); //刷新缓冲区
		sleep(1); //一秒写一次
	}
}
```
以为这样就没事了，那么我们另开一个终端，用`tail -f file.txt`命令查看文件不断append的内容。发现一直没有东西：这个原因就是标准IO的缓冲，因为除了终端设备比如stdout是行缓冲外，其余文件流都是**全缓冲**，缓冲区不满就不写文件，因此要么`fflush(fp)`不断刷新缓冲区，要么直接系统调用IO。

## 进程
### 进程环境
#### main函数
* C程序规定：main函数是程序的**入口**，也是程序的**出口**
* 一般main函数就是两个参数`int main(int argc, char **argv)`，也可以放第三个参数`char **env`环境变量，但是Linux对环境变量做了单独的处理，一般不怎么使用这个参数了
#### 进程的终止
* 5种正常终止，3种异常情况终止必须牢记!!!

* **正常终止**：
  
  * 从main函数返回：
  
    * 也就是`return 0`了
  
  * 调用库函数`exit()`：
  
    * 比如常用的`exit(0); exit(1);`查看man手册中对于exit()库函数的描述：
  
    ```cpp
    void exit(int status);
    //The  exit() function causes normal process termination and 
    //the value of status & 0377 is returned to the parent (see wait(2)).
    ```
	  * 可以看到status是个int，显然这个范围非常大，因此返回地时候也是用了个mask掩码一下，**只取低8位**。
  	  * return的值和exit的返回值是给谁看？答案是**父进程**，谁创建了main这个进程谁就是父进程，比如我们在shell下面`./main`，那么**在这个场景下**，shell进程就是父进程。在shell里可以通过` echo $?`命令查看上一条命令的`exit status`即退出值。
	  * 一般我们**约定**返回值为0，即我们常常写`return 0; exit(0);`作为程序的结束，如果不写的话正常结束的话也返回0.但这只是个约定而已。我们当然可以写成`return 6; exit(3);` 这样`echo $?`查看出来的就是`6 3`。
  
  * 调用系统调用函数`_exit()`或者`_Exit()`:
  
  * 最后一个线程从其启动例程返回:
  
  * 最后一个线程调用`pthread_exit()`函数:
  
* **异常终止**
	* 调用库函数`abort()`
	* 接到一个信号并终止
	* 最后一个线程对其取消请求作出响应
#### 命令行参数分析
#### 环境变量
#### C程序的存储空间布局
#### 关于库
#### 函数的跳转
#### 进程资源的获取和控制

## 并发

多进程并发(信号量)
多线程并发

## IPC(进程间通信)
