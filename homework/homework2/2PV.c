#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<semaphore.h>
typedef struct data{
    int read_counter;
    int write_counter;
    int file;
}data;

data share_data;

int READ_THREAD_NUM = 5;

int stop = 0;

int reader_num = 0;
sem_t lock;
sem_t lock_writer;
sem_t w_or_r;


void* func_read(void *argy){
    while(!stop){
        sem_wait(&lock);
        reader_num += 1;
        share_data.read_counter += 1;
        if(reader_num == 1)
            sem_wait(&w_or_r);
        sem_post(&lock);

        //read file
        int tmp = share_data.file;

        sem_wait(&lock);
        reader_num -= 1;
        if(reader_num == 0)
            sem_post(&w_or_r);
        sem_post(&lock);
        sleep(0.1);
    }
    return NULL;
}

void* func_write(void* argy){
    while(!stop){
        sem_wait(&w_or_r);
	int modify = *(int *)argy;
        share_data.write_counter += 1;
        share_data.file = modify;

        if(share_data.write_counter >= 10000)
            stop = 1;

        sem_post(&w_or_r);
        
	sleep(0.1);
    }
    return NULL;
}

int main(int argc, char * argv[]){
    share_data.read_counter = 0;
    share_data.write_counter = 0;

    sem_init(&lock, 0, 1);
    sem_init(&w_or_r, 0, 1);

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

    sem_destroy(&lock);
    sem_destroy(&w_or_r);

    printf("read thread's count is %d\nwrite thread's count is %d\n", share_data.read_counter, share_data.write_counter);
    return 0;
}
