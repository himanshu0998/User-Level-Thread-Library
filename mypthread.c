// File:	mypthread.c

// List all group members' names: Himanshu Rathi(hr393), Shashwenth Muralidharan(sm2785)
// iLab machine tested on: cpp.cs.rutgers.edu

#include <sys/time.h>
#include <sys/syscall.h>
#include <signal.h>
#include <stdatomic.h>
#include "mypthread.h"
#include <string.h>
#include <time.h>

// INITAILIZE ALL YOUR VARIABLES HERE
//#define MEM SIGSTKSZ
#define MEM 16384
#define MAX_TCBS 5000
#define QSIZE 5000
#define TIME_QUANTUM 5
#define QLEVELS 4


//Time(in microsec) after which we should put all the threads in the top queue 
#define MLFQ_TIMEELAPSED 25

static mypthread_t threadId = 0;
static mypthread_t currThread = 1;
tcb* threadList[MAX_TCBS];
tcb* exitThread;
struct itimerval timer;
my_queue* running_q[QLEVELS];
my_queue* finished_q;
static int curr_q_num = 0;
int running_q_initialized = 0;
int MutexOperation = 0;
int SCHED = 0;
struct timespec start, end;

static void schedule();
static void sched_PSJF();
static void sched_RR();
static void sched_MLFQ();
void boot_timer();
void sigg_handler();
void disable_timer();
void enable_timer();

// YOUR CODE HERE

//Function to initialize the Queue and return the pointer to it
// Basic Structure of Queue is referenced from tutorialspoint and have implemented based on the program requirements
my_queue* initializequeue(mypthread_t size){
    my_queue* q = (my_queue*)malloc(sizeof(my_queue));
    q->front = 0;
    q->max_size = size;
    q->curr_size = 0;
    q->back = q->curr_size-1;
    q->list = (mypthread_t*)malloc(size*sizeof(mypthread_t));
    return q;
}

//Function to enqueue the thread id in the queue and update its parameters
void push(my_queue* q, mypthread_t key){
    q->list[q->back+1] = key;
    q->back = q->back + 1;
    q->curr_size = q->curr_size+1;
}

//Function to pop the first element from the queue and update its parameters
mypthread_t pop(my_queue* q){
    int front = q->list[q->front];
    q->curr_size = q->curr_size-1;
    q->front = q->front + 1;
	return front;
}

//Function to reset the queue parameters
void clearQ(my_queue* q)
{
    q->front = 0;
    q->curr_size = 0;
    q->back = q->curr_size - 1;
}

//Function to display the queue parameters while debugging
void displayQparameters(my_queue* q){
    printf("Max Queue Size: %u\n",q->max_size);
    printf("Current Queue Size: %u\n",q->curr_size);
    printf("Queue Front Pointer: %u\n",q->front);
    printf("Queue Back Pointer: %u\n",q->back);
}


/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg)
{
        /*Initializing the running queue only once at the start.. at the same time initialize the SCHED variable 
		to the correct number based on the command line Definition*/
		if(running_q_initialized == 0){
			
			#ifdef PSJF
			  	SCHED = 1;
			#endif

			#ifdef MLFQ
			  	SCHED = 2;
			#endif

			if(SCHED==0 || SCHED==1)
			  running_q[0] = initializequeue(QSIZE);
			else{
				for(int i=0;i<QLEVELS;i++)
					running_q[i] = initializequeue(QSIZE);
			}
			finished_q = initializequeue(QSIZE);
			running_q_initialized = 1;
		}

       /*
	   If Scheduler, exit and main context is not already Initialized -- initialize them first, assign the memmory
	   0th tcb will always be the Schedulers TCB in the Global ThreadList Array
	   1st tcb will be the Main TCB in the Global ThreadList Array

	   ExitThread's context will be the UC_Link of the created threads Context
	   Boot timer will trigger the Virtual timer with a SIGVTALRM and set the timer to the time quantum defined above 
	   */
	   	if(threadId==0){

		    //Initializing schedulers context
			tcb* scheduler_tcb=(tcb*)malloc(sizeof(tcb));
			scheduler_tcb->tid=threadId++;
			getcontext(&scheduler_tcb->tcontext);
			scheduler_tcb->tcontext.uc_link = NULL;
			scheduler_tcb->tcontext.uc_stack.ss_sp = malloc(MEM);
			scheduler_tcb->tcontext.uc_stack.ss_size = MEM;
			scheduler_tcb->tcontext.uc_stack.ss_flags = 0;
			if(scheduler_tcb->tcontext.uc_stack.ss_sp==NULL){
			  printf("ERROR unable to allocate memory\n");
			  exit(1);
			}
			makecontext(&scheduler_tcb->tcontext, (void (*)()) &schedule, 0);
			threadList[threadId-1]=scheduler_tcb;

			//Initializing Main
			tcb* main_tcb=(tcb*)malloc(sizeof(tcb));
			main_tcb->tid=threadId++;
            getcontext(&main_tcb->tcontext);
			
			main_tcb->tcontext.uc_link = NULL;
			main_tcb->tcontext.uc_stack.ss_sp = malloc(MEM);
			main_tcb->tcontext.uc_stack.ss_size = MEM;
			main_tcb->tcontext.uc_stack.ss_flags = 0;
			
			main_tcb->tstatus=READY;
			main_tcb->quantums_used = 0;
			main_tcb->priority = 0;
			threadList[threadId-1]=main_tcb;
			//push(running_q,main_tcb->tid);
			push(running_q[0],main_tcb->tid);

			//Setting up exit context
            //Exit context would be set as the uclink for all the threads
			exitThread = (tcb*)malloc(sizeof(tcb));
			getcontext(&exitThread->tcontext);
			exitThread->tcontext.uc_link = NULL;
			exitThread->tcontext.uc_stack.ss_sp = malloc(MEM);
			exitThread->tcontext.uc_stack.ss_size = MEM;
			exitThread->tcontext.uc_stack.ss_flags = 0;
			makecontext(&exitThread->tcontext,(void*)mypthread_exit,0);

			tcb *nthread = (tcb*)malloc(sizeof(tcb));
			nthread->tid = threadId;
			*thread = threadId;
			threadId++;
			getcontext(&nthread->tcontext);
			nthread->tcontext.uc_link = &exitThread->tcontext;
			nthread->tcontext.uc_stack.ss_sp = malloc(MEM);
			nthread->tcontext.uc_stack.ss_size = MEM;
			nthread->tcontext.uc_stack.ss_flags = 0;
			nthread->tstatus = READY;
			nthread->blockedThread = 0;
			nthread->quantums_used = 0;
			nthread->priority = 0;
			 if (nthread->tcontext.uc_stack.ss_sp == NULL) {
        		printf("Error: Unable to allocate stack memory\n");
				exit(1);
    		}
			makecontext(&nthread->tcontext,(void*)function,1,arg);
			threadList[*thread] = nthread;
			push(running_q[0],nthread->tid);


			//triggering the real timer
			clock_gettime(CLOCK_REALTIME, &start);
			boot_timer();
			//printf("Back to Main\n");
		}
        else
        {
            // YOUR CODE HERE	
            tcb *nthread = (tcb*)malloc(sizeof(tcb));
            nthread->tid = threadId;
            *thread = threadId;
            threadId++;
            getcontext(&nthread->tcontext);
            nthread->tcontext.uc_link = &exitThread->tcontext;
            nthread->tcontext.uc_stack.ss_sp = malloc(MEM);
            nthread->tcontext.uc_stack.ss_size = MEM;
            nthread->tcontext.uc_stack.ss_flags = 0;
            nthread->tstatus = READY;
            nthread->blockedThread = 0;
			nthread->quantums_used = 0;
			nthread->priority = 0;
			if (nthread->tcontext.uc_stack.ss_sp == NULL) {
				printf("Error: Unable to allocate stack memory\n");
				exit(1);
			}
			makecontext(&nthread->tcontext,(void*)function,1,arg);
            //makecontext(&nthread->tcontext,(void (*)())&mypthread_execute,3,nthread, function, arg);
            threadList[*thread] = nthread;
            push(running_q[0],nthread->tid);
			//push(running_q,nthread->tid);
		}
       //printf("Thread with Id: %d Created Successfully\n", *thread);
	   // create a Thread Control Block
	   // create and initialize the context of this thread
	   // allocate heap space for this thread's stack
	   // after everything is all set, push this thread into the ready queue
		//printf("Threads Created Successfuly\n");
	return 0;
};


/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield()
{
	// YOUR CODE HERE
	tcb* current_thread = threadList[currThread];

	//if the thread is already terminated.. simply call the schedulers context
	if(current_thread->tstatus == TERMINATED) {
		setcontext(&threadList[0]->tcontext);
		return 0;
	}

    //else update the status and push in the running queue
	current_thread->tstatus = READY;
	current_thread->quantums_used++;
	swapcontext(&current_thread->tcontext, &threadList[0]->tcontext);
	// change current thread's state from Running to Ready
	// save context of this thread to its thread control block
	// switch from this thread's context to the scheduler's context

	return 0;
};


/* terminate a thread */
void mypthread_exit(void *value_ptr)
{
	// YOUR CODE HERE
	//printf("In Mypthread Exit\n");
	tcb* current_thread = threadList[currThread];
	current_thread->return_value = value_ptr;
	free(current_thread->tcontext.uc_stack.ss_sp);

    //if there is any blocked thread because of the current thread.. 
    //update the status and push it back in the running q 
    mypthread_t blocked_Id= current_thread->blockedThread;
	if (blocked_Id != 0) {
        tcb* blocked_tcb = threadList[blocked_Id];
		curr_q_num = threadList[blocked_Id]->priority;
        blocked_tcb->tstatus = READY;
        push(running_q[curr_q_num], blocked_tcb->tid);
    }
	//printf("Exited Thread: %u\n", currThread);
	current_thread->tstatus=TERMINATED;
	current_thread->quantums_used++;
	push(finished_q, current_thread->tid);

	//printf("Exited Thread: %u -- Quantums Used: %d\n", current_thread->tid,current_thread->quantums_used);
	setcontext(&threadList[0]->tcontext);	
	// preserve the return value pointer if not NULL
	// deallocate any dynamic memory allocated when starting this thread
	
	return;
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr)
{
	// YOUR CODE HERE
	// /printf("In Join Func\n");
	tcb* current_thread = threadList[currThread];
	tcb* join_thread = threadList[thread];
	//printf("Join Current Thread: %u\n", current_thread->tid);
	//printf("Joined Thread: %u\n", join_thread->tid);

	if(join_thread->tstatus!=TERMINATED)
    {
		current_thread->tstatus = BLOCKED;
		join_thread->blockedThread = current_thread->tid;
		current_thread->quantums_used++;
		//printf("Join Current Thread Quatum Used: %d\n", current_thread->quantums_used);
		swapcontext(&current_thread->tcontext, &threadList[0]->tcontext);
	}
	// wait for a specific thread to terminate
	// deallocate any dynamic memory created by the joining thread
	if(value_ptr) 
    {
        *value_ptr = join_thread->return_value;
    }	
	//printf("Exiting Join Func\n");
	return 0;
};


/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	// YOUR CODE HERE
	//initialize data structures for this mutex
	mutex->lock = 0;
	mutex->waiting = initializequeue(QSIZE);
	mutex->lockHolder = 0;
	//printf("Initialized Mutex wit lock:%d, LockHolder: %u\n",mutex->lock, mutex->lockHolder);
	return 0;
};


/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
    //while (__atomic_test_and_set((void * ) & mutex -> lock, __ATOMIC_RELAXED)) {
	MutexOperation = 1;
    //disable_timer();
	while(__sync_lock_test_and_set(&mutex->lock,1)==1){
        
        tcb* scheduler_thread = threadList[0];
        tcb* current_thread = threadList[currThread];
        push(mutex->waiting, current_thread->tid);
        current_thread->tstatus = WAIT;
		current_thread->quantums_used++;
        swapcontext( &current_thread->tcontext, &scheduler_thread->tcontext);
      }

    mutex->lockHolder = threadList[currThread]->tid;
	//enable_timer();
	MutexOperation = 0;
		//printf("After Aquiring-> lock:%d, LockHolder: %u\n",mutex->lock, mutex->lockHolder);
		// use the built-in test-and-set atomic function to test the mutex
		// if the mutex is acquired successfully, return
		// if acquiring mutex fails, put the current thread on the blocked/waiting list and context switch to the scheduler thread
		
		return 0;
};


/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{
    // YOUR CODE HERE
      // update the mutex's metadata to indicate it is unlocked
      // put the thread at the front of this mutex's blocked/waiting queue in to the run queue
      MutexOperation = 1;
	  //disable_timer();
	  //tcb * current_thread = threadList[currThread];
      if (mutex->waiting->curr_size != 0) {
        //mypthread_t top = pop(mutex->waiting);
        //push(running_q, threadList[top]->tid);
        for(int i=mutex->waiting->front;i<=mutex->waiting->back;i++)
        {
           int waiting_thread = mutex->waiting->list[i];
		   curr_q_num  = threadList[waiting_thread]->priority;
           threadList[waiting_thread]->tstatus = READY;
           push(running_q[curr_q_num], threadList[waiting_thread]->tid);
        }
        mutex->lock = 0;
        mutex->lockHolder = 0;
        clearQ(mutex->waiting);
      }
      else{
        mutex->lock = 0;
        mutex->lockHolder = 0;
        clearQ(mutex->waiting);
      }
	  //enable_timer();
	  MutexOperation=0;
};


int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE
	int i = mutex->waiting->list[mutex->waiting->front];
	/*while(i<QSIZE) 
	{
	  free(mutex->waiting->list[i]);
	  i++;
	}*/
	free(mutex->waiting->list);
	// deallocate dynamic memory allocated during mypthread_mutex_init
	return 0;
};


/* scheduler */
static void schedule()
{
	// YOUR CODE HERE
	//currThread = 0;
	// each time a timer signal occurs your library should switch in to this context
	
	// be sure to check the SCHED definition to determine which scheduling algorithm you should run
	//   i.e. RR, PSJF or MLFQ
	//printf("Scheduler is Called\n");
	if(threadList[currThread]->tstatus == WAIT)
	   threadList[currThread]->quantums_used--;
	threadList[currThread]->quantums_used++;
	if(SCHED==2){
		//printf("MLFQ called\n");
		sched_MLFQ();
	}
	else if(SCHED==1){
		//printf("PSJF called\n");
		sched_PSJF();
	}
	else{
		//printf("RR called\n");
		sched_RR();
	}
		//sched_MLFQ();
	//printf("Exiting Schedule Function\n");
	return;
}


/* Round Robin scheduling algorithm */
static void sched_RR()
{
	// YOUR CODE HERE

	//printf("Attempted Round Robin\n");

	if(running_q[curr_q_num]->curr_size==0) 
    {
		printf("Empty Running Queue\n");
	}
    else
    {

        //check the status of the thread at the top of the Queue
        // If finished -> remove from the Queue, else deque and enque it
        //printf("Round Robin Current Thread B4 dequeue: %u \n",currThread);
        if(threadList[running_q[curr_q_num]->list[running_q[curr_q_num]->front]]->tstatus==TERMINATED)
        {
            pop(running_q[curr_q_num]);
        }
        else if(threadList[running_q[curr_q_num]->list[running_q[curr_q_num]->front]]->tstatus==WAIT) 
        {
            pop(running_q[curr_q_num]);
        }
        else if(threadList[running_q[curr_q_num]->list[running_q[curr_q_num]->front]]->tstatus==BLOCKED)
        {
            pop(running_q[curr_q_num]);
        }
        else
        {
            push(running_q[curr_q_num], running_q[curr_q_num]->list[running_q[curr_q_num]->front]);
            pop(running_q[curr_q_num]);
        }
        /*
        printf("Printing Queue in RR:\n");
        for(int i=running_q[0]->front;i<=running_q[0]->back;i++)
        printf("%u --> ", running_q[0]->list[i]);
        printf("\n");
        */
        currThread = running_q[curr_q_num]->list[running_q[curr_q_num]->front];
        //printf("Round Robin Current Thread After Dequeue: %u \n",currThread);
        //Now set the context to the thread present in the running thread
		threadList[currThread]->tstatus = RUNNING;
        setcontext(&threadList[currThread]->tcontext);
        // Your own implementation of RR
        // (feel free to modify arguments and return types)
	}
	return;
}


/* Preemptive PSJF (STCF) scheduling algorithm */
static void sched_PSJF()
{
	// YOUR CODE HERE
	//printf("In PSJF\n");
	if(running_q[curr_q_num]->curr_size==0) 
    {
		printf("Empty Running Queue\n");
	}
	else
	{
		//If the thread is not active pop it out of the running que
		//Blocked Threads would be added to the running queue once the Joining thread exists
		//Waiting threads would be added to the running queue once the thread holding the lock releases it
		int qfront =  running_q[curr_q_num]->list[running_q[curr_q_num]->front];
		int curr_thread_status = threadList[qfront]->tstatus;
		
		if(curr_thread_status==TERMINATED || curr_thread_status==WAIT||curr_thread_status==BLOCKED)
            pop(running_q[curr_q_num]);
		//Based on the #quantums used by the threads during execution.. prioritize the thread who has used least amount of time time quantums 
		int n = running_q[curr_q_num]->curr_size;
		for (int i = running_q[curr_q_num]->front; i < n - 1; i++)
		{
			for (int j = running_q[curr_q_num]->front; j < n - i - 1; j++)
			{
				if (threadList[running_q[curr_q_num]->list[j]]->quantums_used > threadList[running_q[curr_q_num]->list[j+1]]->quantums_used)
				{
					mypthread_t temp = running_q[curr_q_num]->list[j];
					running_q[curr_q_num]->list[j] = running_q[curr_q_num]->list[j + 1];
					running_q[curr_q_num]->list[j + 1] = temp;
				}
			}
		}
		
		/*
		printf("Printing Queue in SJF:\n");
        for(int i=running_q[0]->front;i<=running_q[0]->back;i++)
        	printf("%u --> ", running_q[0]->list[i]);
        printf("\n");
		*/
		currThread = running_q[curr_q_num]->list[running_q[curr_q_num]->front];
		threadList[currThread]->tstatus = RUNNING;
        setcontext(&threadList[currThread]->tcontext);
	}
	// Your own implementation of PSJF (STCF)
	// (feel free to modify arguments and return types)

	return;
}

/* Preemptive MLFQ scheduling algorithm */
/* Graduate Students Only */
static void sched_MLFQ() {
	// YOUR CODE HERE

	

	clock_gettime(CLOCK_REALTIME, &end);
	unsigned long int diff = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
	//printf("Difference: %lu\n", diff);
	int flag = 0;
	
	//Priority Inversion: After a certain execution time .. here MLFQ_TIMEELAPSED..put all the active threads
	//in the que with highest priority and update their priorities accordingly
	MutexOperation = 1;
	if(diff>=MLFQ_TIMEELAPSED)
	{
		/*
		for(int m=0;m<QLEVELS;m++)
		{
			printf("Queue %d: ",m);
			if(running_q[m]->curr_size!=0){
				for(int x = running_q[m]->front;x <= running_q[m]->back;x++)
				    printf("%u --> ",running_q[m]->list[x]);
				printf("\n");
			}
		}*/
	 	flag=1;
	    int k = 1;
	    while(k<QLEVELS)
	 	{
	 		if(running_q[k]->curr_size>0)
			{
				for(int j=running_q[k]->front;j<=running_q[k]->back;j++)
				{
					int status = threadList[running_q[k]->list[j]]->tstatus;
					if(status!=TERMINATED && status!=WAIT && status!=BLOCKED)
	 				{
						//mypthread_t qfront = pop(running_q[i]);
						//push(running_q[0], qfront);
						threadList[running_q[k]->list[j]]->priority=0;
						push(running_q[0],threadList[running_q[k]->list[j]]->tid);
	 				}
				}
				clearQ(running_q[k]);	
			}
	  		k++;
	 	}
	 	clock_gettime(CLOCK_REALTIME, &start); 
	}
	MutexOperation=0;
	

	int curr_qlevel;
	if(flag)
	   curr_qlevel = 0;
	else
	   curr_qlevel = threadList[currThread]->priority;
	int qfront =  running_q[curr_qlevel]->list[running_q[curr_qlevel]->front];
	int curr_thread_status = threadList[qfront]->tstatus;

	if(curr_thread_status==TERMINATED || curr_thread_status==BLOCKED || curr_thread_status==WAIT)
	{
		pop(running_q[curr_qlevel]);
	}
	else if(curr_qlevel>=QLEVELS-1)
	{
		push(running_q[curr_qlevel], qfront);
		pop(running_q[curr_qlevel]);
	}
	else{
		threadList[qfront]->priority++;
		push(running_q[curr_qlevel + 1], qfront);
		pop(running_q[curr_qlevel]);
	}
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	//reset the timer and run the front thread of every level with a dynamic time quantum
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval = timer.it_value;
	setitimer(ITIMER_VIRTUAL,&timer, NULL);

	for(int i=0;i<QLEVELS;i++)
	{
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = TIME_QUANTUM*(i+1);
		timer.it_interval = timer.it_value;
		setitimer(ITIMER_VIRTUAL,&timer, NULL);
		//printf("Level: %d -- Time quantum: %ld\n",i+1,timer.it_value.tv_usec);
		if(running_q[i]->curr_size>0)
		{
			curr_q_num = i;
			currThread = running_q[curr_q_num]->list[running_q[curr_q_num]->front];
			threadList[currThread]->tstatus = RUNNING;
        	setcontext(&threadList[currThread]->tcontext);
			//swapcontext(&threadList[0]->tcontext,&threadList[currThread]->tcontext);
		}
	}
	return;
}


void boot_timer()
{
	struct sigaction sigac;
	memset(&sigac,0, sizeof(sigac));
	sigac.sa_handler = &sigg_handler;
	sigaction(SIGVTALRM, &sigac, NULL);
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = (TIME_QUANTUM*1000)%1000000;
	timer.it_interval = timer.it_value;
	//printf("Timer sec: %ld\n",timer.it_interval.tv_sec);
	//printf("Timer Usec: %ld\n",timer.it_interval.tv_usec);
	setitimer(ITIMER_VIRTUAL,&timer, NULL);
}


void disable_timer()
{
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval = timer.it_value;
	setitimer(ITIMER_VIRTUAL,&timer, NULL);
}


void enable_timer()
{
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = (TIME_QUANTUM*1000)%1000000;
	timer.it_interval = timer.it_value;
	setitimer(ITIMER_VIRTUAL,&timer, NULL);
}


void sigg_handler(){
	//printf("Inside the Signal Handler CurrentThread: %u \n", currThread);
	if(currThread!=0)
    {
      if(MutexOperation==0)  
	    swapcontext(&threadList[currThread]->tcontext, &threadList[0]->tcontext);
    }
    else{
		//reset the timer
	}
}