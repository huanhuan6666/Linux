#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<semaphore.h>
sem_t S1;
sem_t S2;
void *fun1()
{
    sem_wait(&S1);
    sem_wait(&S2);
    printf("run fun1\n");
    sleep(0.1);
    sem_post(&S1);
    sem_post(&S2);
    return NULL;
}

void *fun2()
{
    sem_wait(&S2);
    sem_wait(&S1);
    printf("run fun2\n");
    sleep(0.1);
    sem_post(&S1);
    sem_post(&S2);
    return NULL;
}

int main(void)
{
    sem_init(&S1, 0, 1); 
    sem_init(&S2, 0, 1);
    pthread_t th_a, th_b;

    pthread_create(&th_a, NULL, fun1, NULL);
    pthread_create(&th_b, NULL, fun2, NULL);
    
    pthread_join(th_a, NULL);
    pthread_join(th_b, NULL);
    return 0;
}
