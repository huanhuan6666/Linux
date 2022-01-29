> 简单应用进程间通信————消息队列

* 头文件`proto.h`：封装协议，确定消息结构体`msg_st`:
```cpp
#ifndef PROTO_H__
#define PROTO_H__

#define KEYPATH "/etc/services"
#define KEYPROJ 'g'

struct msg_st
{
        long mtype; //协议要遵循msgbuf约定
        char name[30];
        int math;
        int chinese;
};
#endif
```

* 接收方：`rcver.c`，创建和销毁消息队列实例，并且接受消息结构体：
```cpp
#include <stdio.h>
#include <stdlib.h>
#include "proto.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int main()
{       
	    key_t key; //recv为被动端需要先运行
	    int msgid;
	    struct msg_st rbuf;
		  key = ftok(KEYPATH, KEYPROJ); //获取key
	    if(key < 0)
	    {
	      perror("ftok()");
	      exit(1);
	    }
	    msgid = msgget(key, IPC_CREAT|0600);  //由于被动端先运行因此要IPC_CREAT根据key创建消息队列实例
		  if(msgid < 0)
	    {
	      perror("msgget()");
	      exit(1);
	    }
	    while(1)
	    {
	      if(msgrcv(msgid, &rbuf, sizeof(rbuf)-sizeof(long), 0, 0) < 0)//第三个参数是接受到的消息大小而第一个msgtype不属于消息
	      {
		perror("msgrcv()");
		exit(1);
	      }
	      printf("NAME = %s\n", rbuf.name);
	      printf("MATH = %d\n", rbuf.math);
	      printf("CHINESE = %d\n", rbuf.chinese);
	    }
            msgctl(msgid, IPC_RMID, NULL); //销毁实例，由于之前是while1因此无法销毁
            exit(0);
}       
```
创建消息队列后，使用`ipcs`可以看到新创建的消息队列：
![image](https://user-images.githubusercontent.com/55400137/151663020-093ba151-bec0-4c83-b262-0fcd9324bf17.png)


* 发送方：`snder.c`，创建消息结构体向消息队列发送消息：
```cpp
#include <stdio.h>
#include <stdlib.h>
#include "proto.h"
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int main()
{
	key_t key;
        int msgid;
        struct msg_st sbuf;
        key = ftok(KEYPATH, KEYPROJ); //获取和recv一样的key
        if(key < 0)
        {
                perror("ftok()");
                exit(1);
        }
        msgid = msgget(key, 0);  //实例在recv创建了，不需要IPC_CREAT
        if(msgid < 0)
        {
                perror("msgget()");
                exit(1);
        }
	sbuf.mtype = 1; //设置消息结构体
	strcpy(sbuf.name,"ZhangHuan");
	sbuf.math = rand() % 100;
	sbuf.chinese = rand() % 100;
	if(msgsnd(msgid, &sbuf, sizeof(sbuf)-sizeof(long), 0) < 0)//发送消息
	{
		perror("magsnd()");
		exit(1);
	}
	puts("send complete!");
	//msg的销毁由主动方完成
	exit(0);
}

```

* 消息队列与命名管道的比较

1) 消息队列不像命名管道那样只有读者和写者同时存在时才能使用，发送端只要发出消息，就算没有接受端，消息也只是**暂存在**消息队列里，比如我们不打开接受端而只向消息队列里发送三次消息：

![image](https://user-images.githubusercontent.com/55400137/151663049-59d1abf8-de33-4fb6-bfe8-b0931c116374.png)

这时查看`ipcs`可以看到消息队列里有暂存的三条消息：

![image](https://user-images.githubusercontent.com/55400137/151663072-79911e1d-b273-48ea-a517-c61f69c25c09.png)

这时我们再打开接受端，将会**一次性读取三条**消息：

![image](https://user-images.githubusercontent.com/55400137/151663092-1dc84693-7bc5-4d23-afc3-caac72adb1f9.png)

2) 而且消息队列可以有选择地接受数据，`msgrcv`函数的`msgtype`可以实现简单的**接收优先级**，而管道就是纯粹的FIFO队列。
