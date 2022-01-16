> 主要参考APUE，简单整理

## I/O
sysio文件I/O(系统调用I/O) 和 stdio标准I/O

* 系统调用IO**直接**和系统kernel对话，在不同的系统下kernel是不一样的，这时候标准IO的概念就应运而生，标准IO是依赖于系统调用IO来实现的，**屏蔽了和kernel的对话**，相当于抛给程序员一个**接口**，因此**移植性更好**。
比如打开文件，C语言的标准IO提供的是`fopen()`，这个标准IO在Linux环境下依赖的系统IO是`open()`，而在windows环境下依赖的是`openfile()`，不同的系统下C**标准库有不同的实现**，因此如果需要移植性优先使用标准IO。
* C语言的标准IO函数：FILE类型贯穿始终，是一个**struct结构体**的typedef
```cpp
fopen();
fclose()

fputc();
fgetc();
fputs();
fgets();
fread();
fwrite();

fseek();
ftell();
rewind();

fflush();

printf();
scanf();
```

### 文件I/O

## 文件系统

## 并发
多进程并发(信号量)
多线程并发

## IPC(进程间通信)
