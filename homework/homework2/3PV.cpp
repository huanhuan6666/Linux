#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
using namespace std;
class CircleBuffer
{
public:
	CircleBuffer(int k);
	~CircleBuffer();
	void put(int ele);
	void get(int &ele);
private:
	int *buffer;
	sem_t lock; /* 互斥体lock 用于对缓冲区的互斥操作 */
    	int readpos, writepos; /* 读写指针*/
    	sem_t notempty; /* 缓冲区非空 */
    	sem_t notfull; /* 缓冲区未满 */
	int max_size;
};
CircleBuffer::CircleBuffer(int k)
{
	buffer = new int[k];
	sem_init(&lock, 0, 1);
    	sem_init(&notempty, 0, 0);
	sem_init(&notfull, 0, k);
    	readpos = 0;
    	writepos = 0;
	max_size = k;
};
CircleBuffer::~CircleBuffer()
{
	sem_destroy(&lock);
    	sem_destroy(&notempty);
    	sem_destroy(&notfull);
}
void CircleBuffer::put(int ele)
{
	sem_post(&notfull);
	sem_wait(&lock);	
    	/* 写数据,并移动指针 */
   	buffer[writepos] = ele;
    	writepos++;
    	if (writepos >= max_size)
        	writepos = 0;
    	/* 设置缓冲区非空的条件变量*/
    	sem_post(&lock);
	sem_post(&notempty);
}
void CircleBuffer::get(int& ele)
{
	sem_wait(&notempty);
	sem_wait(&lock);
    	/* 读数据,移动读指针*/
    	ele = buffer[readpos];
    	readpos++;
    	if (readpos >= max_size)
    	    readpos = 0;
    	/* 设置缓冲区未满的条件变量*/
    	sem_post(&lock);
	sem_post(&notfull);
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
