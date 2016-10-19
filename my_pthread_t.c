#include <malloc.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "my_pthread_t.h"


int thread_count = 0;
int starvation_counter = 0;

queue ready_q, finish_q;

ucontext_t p,c;

long period_t;

struct itimerval my_timer;

void start_time() 
{
	setitimer(ITIMER_VIRTUAL, &my_timer, 0);
}

void stop_time() 
{
	setitimer(ITIMER_VIRTUAL, 0, 0); //2nd argument is 0, implying disability
}

long int time_stamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL); //Get current time populated in tv
	return 1000000 * tv.tv_sec + tv.usec; //Convert current time to microseconds
}

void queue_init(queue *q)
{
	queue->front = NULL;
	queue->rear = NULL;
	queue->ctr = 0;
}

void enqueue(queue *q, my_pthread_t *thread)
{
	if(q.rear != NULL)
	{
		q->rear->next = thread;
		q->rear = thread;
	}
	else
	{
		q->front = thread;
		q->rear = thread;
	}

	q->ctr++;
}

my_pthread_t* dequeue(queue *q)
{
	my_pthread_t *ret;

	if (q.front == NULL)
		return NULL;
	if (q->ctr == 1)
	{
		ret = q->head;
		q->front = NULL;
		q->rear = NULL;
	}
	if (q->ctr > 1)
	{
		ret = q->head;
		q->head = q->head->next;
	}
	ret->next = NULL;
	q->ctr--;
	return ret;
}

void scheduling_function()
{
	my_pthread_t *curr_thread = sc->current_thread;
	if(curr_thread != NULL)
	{
		int curr_priority = t->priority;
		curr_thread->running_time += QUANTUM;
		if(curr_thread->running_time >= (curr_thread->priority+1)*QUANTUM)
		{
			int new_priority;
			if(new_priority < LEVELS) 
			{
				new_priority = curr_priority + 1;
			}
			else //Priority cannot exceed anymore
			{
				new_priority = LEVELS-1;
			}
			add_thread_to_scheduler(curr_thread, new_priority);

		}
		if(curr_thread->thread_state == YIELDED)
		{
			add_thread_to_scheduler(curr_thread,curr_thread->priority); //add to queue at same priority level
		}
	}
	else //control is here because current thread is NULL
	{
		if (sc->current_thread = get_next_thread_for_scheduler() != NULL) //update current thread
		{
			sc->current_thread->thread_state = READY;
		}
	}

	starvation_counter++;
	if (starvation_counter > THRESHOLD - 1)
	{
		starvation_counter = 0;
		for(int i=0; i < LEVELS; i++)
		{
			if(sc->mlfq[i]->head != NULL)
			{
				my_pthread_t *curr_thread = sc->mlfq[i]->head;
				my_pthread_t *prev_thread = NULL;
				while (curr_thread != NULL)
				{
					long int curr_time = time_stamp();
					if (curr_time - curr_thread->last_exe_tt > AGE_TH)
					{
						if(prev_thread == NULL)
							sc->mlpq[i]->head = curr_thread->next;
						else
							prev_thread = curr_thread->next;

						add_thread_to_scheduler(curr_thread, 0);

					}
					else
					{
						prev_thread = curr_thread;
					}

					curr_thread = curr_thread->next;
				}
			}
		}
	}

	struct itimerval my_timer;
	my_timer.it_value.tv_sec = 0;
	my_timer.it_value.tv_usec = QUANTUM;
	my_timer.it_interval.tv_sec = 0;
	my_timer.it_interval.tv_usec = 0;

	setitimer(ITIMER_VIRTUAL, &mytimer, NULL);

	if (sc->current_thread->first_exe_tt == 0)
	{
		sc->current_thread->first_exe_tt = time_stamp();
	}
	sc->current_thread->last_exe_tt = time_stamp();

	if(curr_thread != NULL)
		swapcontext(&curr_thread->uc, &(sc->current_thread->uc));
	else
	{
		ucontext_t temp;
		swapcontext(&temp, &sc->current_thread->uc);
	}

}

void scheduler_init()
{
	sc = (scheduler*)malloc(sizeof(scheduler));
	sc->mlfq = malloc(LEVELS * sizeof(queue));
	sc->wait = (queue*)malloc(sizeof(queue));
	sc->number_of_threads = 0;
	sc->current_thread = NULL;
	for(int i = 0; i < LEVELS; i++)
	{
		queue_init(sc->mlfq + i);
	}
	queue_init(sc->wait);


}

void my_pthread_init (long period) 
{

	my_timer.it_value.tv_sec = 0;
	my_timer.it_value.tv_usec = period * 1000;
	my_timer.it_interval.tv_sec = 0;
	my_timer.it_interval.tv_usec = period * 1000;
	
	start_time();

	signal(SIGVTALRM,scheduler);

	/*Saving context for main thread*/
	main_t.id = -1;
	if ( getcontext(&(main_t.uc)) == -1) 
	{
		printf("Error while getting context...exiting\n");
		exit(EXIT_FAILURE);
	}	
	current = &main_t; /* Storing main thread as current*/

}

void add_thread_to_scheduler(my_pthread_t *thread, int priority)
{
	thread->thread_state = READY;
	thread->running_time = 0;
	thread->priority = priority;
	enqueue(&(sc->mlfq[priority]),thread);
	sc->number_of_threads++;
}

my_pthread_t* get_next_thread_for_scheduler()
{
	for(int i = 0;i < LEVELS; i++)
	{
		if (sc->mlfq[i]->head!=NULL)
		{
			my_pthread_t *get = dequeue(&(sc->mlfq[i]->head));
			sc->number_of_threads--;
			return get;
		}
	}

	//printf("Response time for this thread:%ld\n", get->start_time - get->first_exe_tt);

	return NULL; //no threads in MLFQ
}

int my_pthread_create(my_pthread_t *thread, pthread_attr_t *attr, void *(*function)(void*), void *arg)
{
	thread->id = thread_count++;
	if (getcontext(&(thread->uc)) == -1 ) 
	{
		perror("Error while getting context\n");
		exit(1);
	}
	thread->uc->uc_link = NULL;
	thread->uc->uc_stack->ss_sp = malloc(MEM);
	if(thread.uc.uc_stack.ss_sp == 0)
	{
		perror("malloc: Could not allocate stack\n");
		exit(1);
	}
	thread->uc->uc_stack->ss_size = MEM;
	thread->uc->uc_stack->ss_flags = 0;

	thread->start_time = time_stamp();
	thread->first_exe_tt = 0;

	makecontext(&(thread->uc), &function, arg);

	add_thread_to_scheduler(thread,0);

	return 0;
}

void my_pthread_yield()
{
	sc->current_thread->thread_state = YIELDED;
	scheduling_function();
}

void pthread_exit(void *value_ptr)
{
	sc->current_thread->thread_state = FINISHED;
	sc->current_thread->retval = value_ptr;
	sc->current_thread->last_exe_tt = time_stamp();
	scheduling_function();
}

int my_pthread_join(pthread_t thread, void **value_ptr)
{
	my_pthread_yield();
	thread->retval = value_ptr;
}

int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	mutex.lock = 0;
	return 0;
}

int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) 
{
	stop_time();
	while (mutex->lock == 1) 
	{
		start_time();
		gtthread_yield();
		stop_time();
	}
	mutex->lock = 1;
	start_time();
	return 0;
}

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex)
{
	mutex.lock = 0;
	return 0;
}