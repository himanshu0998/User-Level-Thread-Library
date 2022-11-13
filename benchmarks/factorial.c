#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../mypthread.h"

#define DEFAULT_THREAD_NUM 2
pthread_mutex_t   mutex;
int thread_num;
pthread_t *thread;
int n = 10;
long long res = 1;

typedef struct factorial_args
{
    int start;
    int end;
}fact_args;


void factorial(void* arg)
{
    fact_args* args = arg;
    int start = args->start;
    int end = args->end;
    //printf("Start: %d, End: %d\n", start, end);
    for(int i=start;i<=end;i++)
    {
        res = res*i;
        if(n%2!=0 && i==(n-1))
          res = res*(i+1);
          
    }
    //printf("Factorial: %d\n", res);
    pthread_exit(NULL);
}

int verify(void* arg)
{
    for(int i=1;i<=n;i++)
       res = res*i;
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
            if(thread_num>n)
               thread_num = n;
		}
	}

	thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

	pthread_mutex_init(&mutex, NULL);

	struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);

	for(i = 0; i < thread_num; ++i){
		fact_args* args = (fact_args*)malloc(sizeof(fact_args));
        args->start = i * (n/thread_num) + 1;
        args->end = (i+1)*(n/thread_num);
        pthread_create(&thread[i], NULL, &factorial, args);
    }
	for (i = 0; i < thread_num; ++i)
		pthread_join(thread[i], NULL);

	clock_gettime(CLOCK_REALTIME, &end);
        printf("running time: %lu micro-seconds\n", 
	       (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
	printf("res is: %lld\n", res);

	pthread_mutex_destroy(&mutex);
	
    res=1;
    verify(NULL);
    printf("Verified Factorial: %lld\n",res);

	// Free memory on Heap
	free(thread);
	//free(counter);
    
    return 0;
}