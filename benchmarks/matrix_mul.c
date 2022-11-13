#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../mypthread.h"

#define DEFAULT_THREAD_NUM 2
pthread_mutex_t   mutex;
int thread_num;
pthread_t *thread;

#define DEFAULT_THREAD_NUM 2
#define MAX_MATRIX_SIZE 1000
pthread_mutex_t   mutex;
int thread_num;
pthread_t *thread;
int* counter;
long long res = 0;

long long mat_A[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE];
long long mat_B[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE];
long long mat_C[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE];

void mat_mul(void *args)
{
    int i = *((int*) args);
    for(int j=0;j<thread_num;j++)
    {
        for(int k = 0;k<thread_num;k++)
        {
            pthread_mutex_lock(&mutex);
            res += mat_A[i][k] * mat_B[k][j];
            pthread_mutex_unlock(&mutex);
        }
        // mat_C[i][j] = mat_A[i][j] + mat_B[i][j];
        // pthread_mutex_lock(&mutex);
        // res += mat_C[i][j];
        // pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void verify(void *args)
{
    res = 0;
    for(int i=0;i<thread_num;i++)
    {
        for(int j=0;j<thread_num;j++)
        {
            for(int k=0;k<thread_num;k++)
               res+= mat_A[i][k] * mat_B[k][j];
        }
    }
    printf("verified res is: %lld\n", res);
}



int main(int argc, char **argv) {
	
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
	counter = (int*)malloc(thread_num*sizeof(int));
	for (i = 0; i < thread_num; ++i)
		counter[i] = i;

	// initialize pthread_t
	thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

	// initialize data array
	// for (i = 0; i < VECTOR_SIZE; ++i) {
	// 	r[i] = i;
	// 	s[i] = i;
	// }

    //Randomly Initialize the Matrices
    for(int i=0;i<thread_num;i++)
    {
        for(int j=0;j<thread_num;j++)
        {
            mat_A[i][j] = rand()%MAX_MATRIX_SIZE;
            mat_B[i][j] = rand()%MAX_MATRIX_SIZE;
            mat_C[i][j] = 0;
        }
    }

	pthread_mutex_init(&mutex, NULL);

	struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);

	for (i = 0; i < thread_num; ++i)
		pthread_create(&thread[i], NULL, &mat_mul, &counter[i]);

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
	free(counter);
	return 0;
}