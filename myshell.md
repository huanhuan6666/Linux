> 模拟一个shell程序，完成对于**外部命令**的解析执行
> 内部命令是shell程序的一部分，比如`cd, echo, exit`等命令，我们这里没有涉及。只涉及到外部使用命令，即在`/bin/`文件夹下可以找到二进制映像的程序，比如`ls, pwd`等等非常多。


shell的基本原理为：
* 循环输出提示符，接受命令
* 收到命令解析命令，fork出子进程
* 子进程exec切换成命令进程，并按照输入的参数执行
* shell进程wait子进程结束

根据上述的分析，可以得到基本框架：
```cpp
int main()
{
  int pid = 0;
  while(1)
  {
    prompt(); //输出提示符
    getline(); //输入命令
    parse(); //解析命令
    if(0)
    {
        //内部命令不做处理
    }
    else //外部命令
    {
      pid = fork(); //创建子进程
      if(pid < 0)
      {
        perror("fork()");
        exit(1);
      }
      else if(pid == 0) //子进程
      {
        exec(); //切换命令程序
        exit(0); //执行完退出
      }
      else //父进程
        wait(NULL); //等待子进程结束
    }
  }
}
```
* 输出提示符非常简单:

```cpp
void prompt()
{
  printf("myshell-0.1$ ");
}

* 接受命令

按照getline的参数来，需要两个**输出指针**
```cpp
char *bufline = NULL;
size_t bufline_size = 0;
getline(&bufline, &bufline_size, stdin);
```

* 解析命令

解析命令就比较麻烦了，显然我们是想处理成main函数参数那样的，像argv得到一个**字符串指针数组**，这时候就凸显出`glob`函数的强大了，因为glob_t结构体就有和argc和argv类似的结构。
我们先将命令按照分隔符分割成一块块，想要把这一条条字符串直接放到argv里面。而glob()函数里flag可以设置一个参数`GLOB_NOCHECK`，如果没有匹配的模式，那么就返回**原始的模式字符串**，这正是我们想要的。
而且我们需要一条条追加上去，因此需要设置`GLOB_APPEND`，需要注意`GLOB_APPEND`必须在**第二次之后才能使用**，因为第一次就append可能会追加到垃圾内容后面。
可是glob函数是匹配文件名称的啊，
```cpp
#define DELMIS " \t\n"
glob_t res;
void parse (char *line, glob_t *res)
{
  char *token;
  int i = 0;
  while(1)
  {
    token = strsep(&line, DELMIS); //用strsep将命令分隔
    if(token == NULL)
      break;
    if(token[0] == '\0') //解析出空串则继续
      continue;
    glob(token, GLOB_NOCHECK|GLOB_APPEND*i, NULL, res); //res是个glob_t输出指针，第二次开始GLOB_APPEND生效
    i = 1;
  }
}
```

* 子进程切换

之前的操作已经得到了一个字符串数组指针，并且是自停止的，glob_t结构体和main函数参数惊人地相像。
而下面就是exec函数族的选择了，显然只有`execvp()`是最合适的，量身打造属于是。
```cpp
int execvp(const char *file, char *const argv[]); //需要文件名和参数列表的自停止的二级指针
```
execvp需要的是文件名，我们没法用完整的路径名`execv`函数，因为解析出来的命令都是单纯的`ls`而不是`/bin/ls`，只能给个文件名，然后让程序在**环境变量的PATH**中找。
exec函数族有个特点就是，argv参数都要从`argv[0]`即文件名开始，emmm即使是手工写参数的`execl(path, argv0, argv1...)`也是这样，这可能是考虑到了`argv[0]`还可以**换个进程名**。
于是乎，子进程部分的实现如下：
```cpp
execvp(res.gl_pathv[0], res.gl_pathv); //文件名及参数列表
```

这样一个对于shell程序的外部命令解析执行过程的模拟就基本完成了：
从输出提示符，到glob函数解析输入命令，到fork创建进程然后子进程再execvp，最后myshell等待wait子进程的结束并回收。

* 更多

我们知道/etc/passwd文件里存放着每个用户的：用户名:口令:用户标识号:组标识号:注释性描述:主目录:登录Shell。
比如我的`njucs:x:1000:1000:njucs,,,:/home/njucs:/bin/bash`，就是常见的`/bin/bash`。
那么我们其实可以以root身份改变某个用户的**登录shell**为我们自己写的`myshell`映像的路径。

比如创建一个test用户：
![image](https://user-images.githubusercontent.com/55400137/150355183-99438ae3-753e-4b67-aebe-bd24bd733e4a.png)

然后`sudo -i`暂时以root用户身份将文件cp到`/uer/bin/local/myshell`文件。

修改这个用户的登录shell：
![image](https://user-images.githubusercontent.com/55400137/150463448-fe0788b2-66f5-4f62-97c2-1bef21246f89.png)


接下来`ctrl+alt+F3`切换到终端模式，然后登录test用户，就会发现登录shell变成我们写的myshell了：

![image](https://user-images.githubusercontent.com/55400137/150463701-30cd235e-d901-4231-a543-fa5763e55281.png)

由于test用户还什么都没有，ls什么都不显示，但是`whoami`可以看到确实是test用户，并且是我们写的myshell。尴尬的是由于exit是**内部命令**我们没法解析，只能`ctrl C`退出。

* 关于登录shell的
这个登录shell就是个二进制映像而已，完全可以换成别的。和脚本文件的第一行`#!/bin/bash`类似都可以随便改。
比如我们把test用户的登录shell改成`top`程序，即任务管理器：
![image](https://user-images.githubusercontent.com/55400137/150463983-18c86197-0417-4029-9c48-3454e7c7b7fe.png)

之后切换到终端模式登录test用户，就会变成下面这个样子：

![image](https://user-images.githubusercontent.com/55400137/150464307-cd55c33b-dc6c-463a-bfac-efbf83171936.png)
