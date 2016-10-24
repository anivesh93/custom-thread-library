#include <ucontext.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define NUM_THREADS 15
//Stack size of each thread : 16 bytes
#define MEM 16384
#define LEVELS 16
//Quantum duration : 50 ms
#define QUANTUM 50000
//Check for starvation every 50 quanta
#define STARVATION_THRESHOLD 50

//Thread states
typedef enum state 
{
	READY, RUNNING, WAITING, YIELDED, FINISHED
} state;

//Thread structure 
typedef struct my_pthread_t 
{
	ucontext_t uc;
	struct my_pthread_t *next;
	state thread_state;
	long int id;
	int running_time;
	int priority;
	void *return_val;
	long int last_exec_time;
} my_pthread_t;

//Queue structure
typedef struct 
{
	my_pthread_t *front;
	my_pthread_t *rear;
	int ctr;
} queue;

//Mutex structure
typedef struct 
{
  int lock;                            
  my_pthread_t *owner;
  queue *wait;
} my_pthread_mutex_t;

//Queue functions
void queue_init(queue *q);
void enqueue(queue *q, my_pthread_t *thread);
my_pthread_t* dequeue(queue *q);

//my_pthread functions
int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, void *(*function)(void *), void* arg);
void my_pthread_yield();
void my_pthread_exit(void* value_ptr);
int my_pthread_join(my_pthread_t* thread, void** value_ptr);

//Mutex functions
int my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr);
int my_pthread_mutex_lock(my_pthread_mutex_t* mutex);
int my_pthread_mutex_unlock(my_pthread_mutex_t* mutex);
int my_pthread_mutex_destroy(my_pthread_mutex_t* mutex);

void my_pthread_runner(my_pthread_t* thr_node, void *(*f)(void *), void* arg); //Wrapper function to execute functions in threads

//Scheduler functions
void scheduler_init();
void scheduler();
void scheduler_add_thread(my_pthread_t* thr_node, int priority);
my_pthread_t* scheduler_get_next_thread();

long int time_stamp();