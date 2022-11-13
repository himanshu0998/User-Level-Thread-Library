#include<pthread.h>
#include<stdio.h>
#include"../mypthread.h"
#include "pthread.h"
#define DEFAULT_THREAD_NUM 2


/* Global variables */
pthread_mutex_t   mutex;
int thread_num;
int* counter;
pthread_t *thread1,*thread2;
int num=0;

void* func1(){
    for(int i=0;i<100000;i++){
    pthread_mutex_lock(&mutex);
    num++;
    pthread_mutex_unlock(&mutex);
    }
	pthread_exit(NULL);
}
void* func2(){
    for(int i=0;i<100000;i++){
    pthread_mutex_lock(&mutex);
    num--;
    pthread_mutex_unlock(&mutex);
    }
	pthread_exit(NULL);
}

void verify(){
	for(int i=0;i<100000;i++){
    num++;
    }
	for(int i=0;i<100000;i++){
    num--;
    }
	printf("Verified Result is %d\n", num);
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

	
	thread1 = (pthread_t*)malloc(thread_num*sizeof(pthread_t));
    thread2 = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

	pthread_mutex_init(&mutex, NULL);

	struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);

	for (i = 0; i < thread_num; ++i){
		pthread_create(&thread1[i], NULL, &func1, &counter[i]);
        pthread_create(&thread2[i], NULL, &func2, &counter[i]);}
	
	for (i = 0; i < thread_num; ++i){
		pthread_join(thread1[i], NULL);
        pthread_join(thread2[i], NULL);}

	clock_gettime(CLOCK_REALTIME, &end);
        printf("running time: %lu micro-seconds\n", 
	       (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
	printf("res is: %d\n", num);

	pthread_mutex_destroy(&mutex);
	verify();

	// Free memory on Heap
	free(thread1);
    free(thread2);
	free(counter);
	return 0;
}