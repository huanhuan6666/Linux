#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#define ALL 1000000

int n;//the counts of threads
int *be_in;
void * work_func(void *arg)
{
	int id = (int)arg;
	int sample = ALL / n;
	unsigned int seed = time(NULL) + id;
	//srand(time(NULL));
	for(int i=0;i<sample;i++)
	{
		double x = 1.0*rand_r(&seed)/RAND_MAX;
		double y = 1.0*rand_r(&seed)/RAND_MAX;
		//double x = 1.0*rand()/RAND_MAX;
		//double y = 1.0*rand()/RAND_MAX;
		if(x*x + y*y < 1)
			be_in[id]++;
	}
}

int main(int argc, char* argv[])
{
	n = atoi(argv[2]);
	printf("the count of threads is %d\n", n);
	pthread_t worker[n];//n threads
	be_in = malloc(sizeof(int)*n);//each thread's points be in circle
	
	void *work_status[n];
	int count[n];//the num of thread
	for(int i=0;i<n;i++)
	{
		count[i] = i;//the count of each pthread
		pthread_create(&worker[i], NULL, work_func, (void*)count[i]);
	}
	for(int i=0;i<n;i++)
	{
		pthread_join(worker[i], &work_status[i]);
	}
	double sum_in = 0;
	for(int i=0;i<n;i++)
		sum_in += be_in[i];
	printf("the pi is %f\n", 4.0*sum_in/ALL);
	return 0;
	
}
