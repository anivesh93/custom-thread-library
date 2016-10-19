#include <malloc.h>
#include <stdio.h>
#include <signal.h>
#include <ucontext.h>

// 64 KB Stack Memory
#define MEM 64 * 1024
#define LEVELS 4
#define QUANTUM (50/1000) * 1000000
#define THRESHOLD 50
#define AGE_TH 50 * (50/1000) * 1000000

ucontext_t p,c;

typedef struct 
{
	long int id;
	ucontext_t uc;
	void *retval;
	struct mypthread_t * next;
	state thread_state;
	int num_runs;
	int running_time;
	int priority;
	long int start_time;
	long int first_exe_tt;
	long int last_exe_tt;
	long int end_tt;
} my_pthread_t;

typedef enum 
{
	READY, RUNNING, WAITING, YIELDED, FINISHED
} state;

typedef struct
{
	struct node *front;
	struct node *rear;
	int ctr;
} queue;

typedef struct 
{
	queue *mlfq;
	queue *wait;
	int number_of_threads;
	my_pthread_t *current_thread;
	my_pthread_t *main_thread;
} scheduler;

typedef struct 
{
	my_pthread_t *caller;
	queue *wait;
} my_pthread_mutex_t;

void queue_init(queue *q);

void enqueue(queue *q, my_pthread_t *thread);

my_pthread_t* dequeue(queue *q)

void my_pthread_init(long period);

int my_pthread_create(my_pthread_t *thread, pthread_attr_t *attr, void *(*function)(void*), void *arg);

void my_pthread_yield();

void pthread_exit(void *value_ptr);

int my_pthread_join(pthread_t thread, void **value_ptr);

typedef struct {
	//Locked = 1; Unlocked = 0
	int lock; 
} my_pthread_mutex_t;

int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

long int time_stamp()
