// File:	mypthread_t.h

// List all group members' names:
// iLab machine tested on:

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* in order to use the built-in Linux pthread library as a control for benchmarking, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
//#include "myQueue.h"

typedef uint mypthread_t;

typedef enum thread_status{
    READY, RUNNING, BLOCKED,WAIT,TERMINATED
}thread_status;

	/* add important states in a thread control block */
typedef struct threadControlBlock
{
	// YOUR CODE HERE	
	mypthread_t tid;
    ucontext_t tcontext;
    thread_status tstatus;
	void* return_value;
	mypthread_t blockedThread;
	int quantums_used;
	int priority;
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

} tcb;



// Feel free to add your own auxiliary data structures (linked list or queue etc...)
typedef struct my_queue{
    mypthread_t front;
    mypthread_t back;
    mypthread_t max_size;
    mypthread_t curr_size;
    mypthread_t* list;
}my_queue;

/* mutex struct definition */
typedef struct mypthread_mutex_t
{
	int lock;
	mypthread_t lockHolder;
	// YOUR CODE HERE
	my_queue* waiting;
} mypthread_mutex_t;


/* Function Declarations: */

my_queue* initializequeue(mypthread_t size);

void push(my_queue* q, mypthread_t key);

mypthread_t pop(my_queue* q);

void displayQparameters(my_queue* q);

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initialize a mutex */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire a mutex (lock) */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release a mutex (unlock) */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy a mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

/*Function to start with the thread execution*/
void mypthread_execute(tcb* th, void *(*mypthread_func)(void*), void * arg);

/*Function to Boot the timer*/
//void boot_timer();
//void sigg_handler();

//static void schedule();
//static void sched_RR();

#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif
