> 用线程互斥量和条件变量实现一个简单的线程间管道，来解决读者写者问题，（可以注册多个读者写者

* 头文件`mypipe.h`代码如下：
```cpp
#ifndef MYPIPE_H__
#define MYPIPE_H__

#define PIPESIZE 1024
#define MYPIPE_READ 0x00000001UL
#define MYPIPE_WRITE 0x00000002UL
typedef void mypipe_t; // void是为了隐藏数据结构void*万能指针

int mypipe_register(mypipe_t*, int opmap); //omap就是位图

int mypipe_unregister(mypipe_t*, int opmap);

mypipe_t* mypipe_init();

int mypipe_read(mypipe_t* , void*buf, int maxsize);

int mypipe_write(mypipe_t*, const void* buf, int size);

int mypipe_destroy(mypipe_t *);
#endif
```
两点就在于注册时的**位图**，以及对原始数据结构的隐藏。

* 源文件`mypipe.c`代码如下：
```cpp
#include "mypipe.h"
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

struct mypipe_st //pipe的行为和队列完全一致
{
    int head;
    int tail;
    char data[PIPESIZE];
    int datasize;
    int count_rd; //读者个数
    int count_wr; //写者个数
    pthread_mutex_t mut;
    pthread_cond_t cond; //通知法
};
int mypipe_register(mypipe_t* ptr, int opmap) //注册身份
{
    struct mypipe_st *mp =  ptr;
    pthread_mutex_lock(&mp->mut);
    if(opmap & MYPIPE_READ)
      mp->count_rd++;
    if(opmap & MYPIPE_WRITE)
                  mp->count_wr++;
    pthread_cond_broadcast(&mp->cond); //注册后叫醒其他cond_wait的进程
    while(mp->count_rd <= 0 || mp->count_wr <= 0) //读者写者都至少有一个
      pthread_cond_wait(&mp->cond, &mp->mut); //等待其他操作该管道的人注册读者或者写者

    pthread_mutex_unlock(&mp->mut);
    return 0;
}

int mypipe_unregister(mypipe_t* ptr, int opmap) //注销身份
{
    struct mypipe_st *mp =  ptr;
          pthread_mutex_lock(&mp->mut);
          if(opmap & MYPIPE_READ)
                  mp->count_rd--;
          if(opmap & MYPIPE_WRITE)
                  mp->count_wr--;
    pthread_cond_broadcast(&mp->cond);//防止读者干等
    pthread_mutex_unlock(&mp->mut);
    return 0;
}

mypipe_t* mypipe_init()
{
    struct mypipe_st* mp = NULL;
    mp = malloc(sizeof(*mp));
    if(mp == NULL)
    {
      fprintf(stderr, "malloc failed\n");
      return NULL;
    }
    mp->head = 0;
    mp->tail = 0;
    mp->datasize = 0;
    mp->count_rd = 0;
    mp->count_wr = 0;
    pthread_mutex_init(&mp->mut, NULL);
    pthread_cond_init(&mp->cond, NULL);
    return mp;
}
int mypipe_readbyte(struct mypipe_st* me, char *buf)
{
    if(me->datasize <= 0)
      return -1;
    *buf = me->data[me->head];
    me->head = (me->head+1)%PIPESIZE;
    me->datasize--;
    return 0;
}
int mypipe_read(mypipe_t* ptr, void*buf, int maxsize)
{
    struct mypipe_st *mp =  ptr;
    int i = 0;
    pthread_mutex_lock(&mp->mut); //上锁
    while(mp->datasize<=0 && mp->count_wr>0) //没有可读数据并且还有写者
      pthread_cond_wait(&mp->cond, &mp->mut); //解锁等待通知

    if(mp->datasize <= 0 && mp->count_wr <= 0)
    {
      pthread_mutex_unlock(&mp->mut); //没有写者就可以直接寄了
      return 0;
    }
    for(i = 0; i< maxsize; i++)//一次读一个字节
      if(mypipe_readbyte(mp, buf+i) != 0)
        break;
    pthread_cond_broadcast(&mp->cond); //通知写者的多个线程开始抢锁
    pthread_mutex_unlock(&mp->mut); //解锁
    return i; //读到的字节数
}
int mypipe_writebyte(struct mypipe_st* me, const char *buf)
{
    if(me->datasize >= PIPESIZE)
            return -1;
    me->data[me->head] = *buf;
    me->head = (me->head+1)%PIPESIZE;
    me->datasize++;
    return 0;
}

int mypipe_write(mypipe_t* ptr, const void* buf, int size)
{
    struct mypipe_st *mp =  ptr;
    int i = 0;
    pthread_mutex_lock(&mp->mut); //上锁
    while(mp->datasize>=PIPESIZE && mp->count_rd>0) //写满了并且还有读者
            pthread_cond_wait(&mp->cond, &mp->mut); //解锁等待通知

    if(mp->datasize>=PIPESIZE && mp->count_rd <= 0)
    {
            pthread_mutex_unlock(&mp->mut); //没有读者就可以直接寄了
            return 0;
    }
    for(i = 0; i< size; i++)//一次写一个字节
            if(mypipe_writebyte(mp, buf+i) != 0)
                    break;
    pthread_cond_broadcast(&mp->cond); //通知读者的多个线程开始抢锁
    pthread_mutex_unlock(&mp->mut); //解锁
    return i; //读到的字节数

}

int mypipe_destroy(mypipe_t *ptr)
{
    struct mypipe_st *mp =  ptr;
    pthread_mutex_destroy(&mp->mut);
    pthread_cond_destroy(&mp->cond);
    free(ptr);
    return 0;
}
```
* 互斥量和条件变量的使用套路也再次展示了一遍，互斥量需要配合条件变量使用（**解锁等待**），放到while循环里
* 互斥量一般就解决临界区的重入问题，保证**互斥访问**临界区；而条件变量则在等待**条件改变**时通过`pthread_cond_broadcast`或者`pthread_cond_signal`**通知**别的线程。

