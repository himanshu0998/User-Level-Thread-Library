#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../mypthread.h"

#define DEFAULT_THREAD_NUM 2
pthread_mutex_t   mutex;
int thread_num;
pthread_t *thread;
int i=0;
const int n = 200;
int m = n;
long long res = 0;

void fibonacci_sum(void* args)
{
    if(i>n)
        return;
    
    pthread_mutex_lock(&mutex);
    res = res + i;
    i++;
    pthread_mutex_unlock(&mutex);
    return fibonacci_sum(NULL);
    // while(i<n)
    // {

    //     res = res+i;
    //     i++;
    // }
}

void verify()
{
    res = 0;
    i = 0;
    while(i<=n)
    {
        res = res + i;
        i++;
    }
    printf("verified res is: %lld\n", res);
}


int main(int argc, char **argv)
{
    //printf("Verified Factorial: %d\n", verify(NULL));
    
    int i = 0;

	if (argc == 1) {
		thread_num = DEFAULT_THREAD_NUM;
	} else {
		if (argv[1] < 1) {
			printf("enter a valid thread number\n");
			return 0;
		} else {
			thread_num = atoi(argv[1]);
		}
	}

	// initialize counter
	/*
    counter = (int*)malloc(thread_num*sizeof(int));
	for (i = 0; i < thread_num; ++i)
		counter[i] = i;
    */
	// initialize pthread_t
	thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

	pthread_mutex_init(&mutex, NULL);

	struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);

	for(i = 0; i < thread_num; ++i){
        pthread_create(&thread[i], NULL, &fibonacci_sum, NULL);
    }
	for (i = 0; i < thread_num; ++i)
		pthread_join(thread[i], NULL);

	clock_gettime(CLOCK_REALTIME, &end);
        printf("running time: %lu micro-seconds\n", 
	       (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
	printf("res is: %lld\n", res);

	pthread_mutex_destroy(&mutex);
	
    verify(NULL);

	// Free memory on Heap
	free(thread);
    
    return 0;
}