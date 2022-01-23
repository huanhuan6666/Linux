#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

typedef struct data{
    int read_counter;
    int write_counter;
    int file;
}data;

data share_data;

int READ_THREAD_NUM = 5;

int stop = 0;
pthread_rwlock_t rwlock;
pthread_mutex_t lock;

void* func_read(void *ptr){
    while(!stop){
        pthread_rwlock_rdlock(&rwlock);

        pthread_mutex_lock(&lock);
        share_data.read_counter += 1;
        pthread_mutex_unlock(&lock);
        //read file
        int tmp = share_data.file;

        pthread_rwlock_unlock(&rwlock);
        sleep(0.1);
    }
    return NULL;
}

void* func_write(void *argy){
    int modify = *(int*)argy;
    while(!stop){
        pthread_rwlock_wrlock(&rwlock);
	
        share_data.write_counter += 1;
        share_data.file = modify;
        if(share_data.write_counter >= 10000)
            stop = 1;
        pthread_rwlock_unlock(&rwlock);
        sleep(0.1);

    }
    return NULL;
}

int main(int argc, char * argv[]){
    share_data.read_counter = 0;
    share_data.write_counter = 0;

    pthread_rwlock_init(&rwlock, NULL);

    pthread_mutex_init(&lock, 0);

    pthread_t readers[READ_THREAD_NUM];
    pthread_t writer;

    for(int i = 0; i < READ_THREAD_NUM; i++){
        pthread_create(&readers[i], NULL, func_read, NULL);
    }
    int modify = 233;
    pthread_create(&writer, NULL, func_write, (void *) &modify);

    for(int i = 0; i < READ_THREAD_NUM; i++)
        pthread_join(readers[i], NULL);
    pthread_join(writer, NULL);

    pthread_rwlock_destroy(&rwlock);
    pthread_mutex_destroy(&lock);
    printf("read thread's count is %d\nwrite thread's count is %d\n", share_data.read_counter, share_data.write_counter);
    return 0;
}
