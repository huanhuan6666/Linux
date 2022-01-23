#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
using namespace std;
class CircleBuffer
{
public:
	CircleBuffer(int k);
	void put(int ele);
	void get(int &ele);
private:
	int *buffer;
	pthread_mutex_t lock; /* 互斥体lock 用于对缓冲区的互斥操作 */
    	int readpos, writepos; /* 读写指针*/
    	pthread_cond_t notempty; /* 缓冲区非空的条件变量 */
    	pthread_cond_t notfull; /* 缓冲区未满的条件变量 */
	int max_size;
};
CircleBuffer::CircleBuffer(int k)
{
	buffer = new int[k];
	pthread_mutex_init(&lock, NULL);
    	pthread_cond_init(&notempty, NULL);
    	pthread_cond_init(&notfull, NULL);
    	readpos = 0;
    	writepos = 0;
	max_size = k;
};
void CircleBuffer::put(int ele)
{
	pthread_mutex_lock(&lock);	
    	if ((writepos + 1) % max_size ==readpos)/* 等待缓冲区未满*/
    	{
    	    	pthread_cond_wait(&notfull, &lock);//进入等待后会释放互斥，使其他线程进入互斥改变状态
    	}
    	/* 写数据,并移动指针 */
   	buffer[writepos] = ele;
    	writepos++;
    	if (writepos >= max_size)
        	writepos = 0;
    	/* 设置缓冲区非空的条件变量*/
    	pthread_cond_signal(&notempty);
    	pthread_mutex_unlock(&lock);
}
void CircleBuffer::get(int& ele)
{
	pthread_mutex_lock(&lock);
    	if (writepos == readpos)//此时缓冲区为空
    	{
    	    pthread_cond_wait(&notempty, &lock);
    	}
    	/* 读数据,移动读指针*/
    	ele = buffer[readpos];
    	readpos++;
    	if (readpos >= max_size)
    	    readpos = 0;
    	/* 设置缓冲区未满的条件变量*/
   	pthread_cond_signal(&notfull);
    	pthread_mutex_unlock(&lock);
}


CircleBuffer A(10);
void *producer(void *data)
{
    int n;
    for (n = 0; n < 15; n++)
    {
        printf("%d --->\n", n);
        A.put(n);
	sleep(0.1);
    } 
    A.put(-1);
    return NULL;
}

void *consumer(void *data)
{
    int d;
    while (1)
    {
        A.get(d);
        if (d == -1)
            break;
        printf("--->%d \n", d);
	sleep(0.1);
    }
    return NULL;
}

int main(void)
{
    pthread_t th_a, th_b;

    pthread_create(&th_a, NULL, producer, 0);
    pthread_create(&th_b, NULL, consumer, 0);
    
    pthread_join(th_a, NULL);
    pthread_join(th_b, NULL);
    return 0;
}
