//Name : Anivesh Baratam
//RUID : 167006747
//NETID : ab1517
//ILAB MACHINE : singleton.cs.rutgers.edu
#include "my_pthread_t.h"

static long int thread_counter = 0;
static long int starvation_counter = 0;
static my_pthread_t *threads;
static my_pthread_mutex_t *m;
static int var_w_mutex = 50;
static int var_wo_mutex = 50;
//If first_thread == 0, then call scheduler_init()
static int first_thread = 0;

//Initialized in scheduler_init()
static queue *mlfq, *wait;
static my_pthread_t *current_thread, *main_thread;
static long int number_of_threads;

//Store return val on thread exit or join
static void *value_ptr;


// Function to record start_time, end_time, start_of_execution, end_of_execution
long int time_stamp()
{
	 struct timeval tv;
	 gettimeofday(&tv, NULL); //Get current time populated in tv
	 return 1000000 * tv.tv_sec + tv.tv_usec; //Convert current time to microseconds
}


// START OF QUEUE FUNCTIONS********************************************************************************
void queue_init(queue *q) 
{
	q->front = NULL;
	q->rear = NULL;
	q->ctr = 0;
}

void enqueue(queue *q, my_pthread_t *thread) 
{
	if(q->rear != NULL)
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
	my_pthread_t *temp;
	if (q->front == NULL) 
		return NULL;
	else if (q->ctr == 1) 
	{
		temp = q->front;
		q->front = NULL;
		q->rear = NULL;
	} 
	else if (q->ctr > 1)
	{
		temp = q->front;
		q->front = q->front->next;
	}
	temp->next = NULL;
	q->ctr--;
	return temp;
}
// END OF QUEUE FUNCTIONS**********************************************************************************

//Scheduler function()
void scheduler()
{
	struct itimerval my_timer;
    ucontext_t uc_temp; //To store the context of the main thread
    
    //clear the timer
    my_timer.it_value.tv_sec = 0;
    my_timer.it_value.tv_usec = 0;
    my_timer.it_interval.tv_sec = 0;
    my_timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &my_timer, NULL);
   
    my_pthread_t* temp = current_thread;
	if (temp != NULL)
	{
		int current_priority = temp->priority;
		temp->running_time += QUANTUM;

		if (temp->thread_state == WAITING)
		{
			//pass
			current_thread = scheduler_get_next_thread();
			if (current_thread != NULL)
				current_thread->thread_state = RUNNING;
		}
		else if (temp->thread_state == FINISHED)
		{
			//pass
			current_thread = scheduler_get_next_thread();
			if (current_thread != NULL)
				current_thread->thread_state = RUNNING;
		}
		else if (temp->thread_state == YIELDED)
		{
			//put the thread back into the original queue
			scheduler_add_thread(temp, temp->priority);
			current_thread = scheduler_get_next_thread();
			if (current_thread != NULL)
				current_thread->thread_state = RUNNING;
		}
		else if (temp->running_time >= (current_priority+1)*QUANTUM) //Thread ran through its time quantum slice
		{
			int new_priority;
			if (current_priority + 1 < LEVELS) 
				new_priority = current_priority + 1;
			else //Priority cannot exceed anymore
				new_priority = current_priority;
			scheduler_add_thread(temp, new_priority);

			current_thread = scheduler_get_next_thread();
			if (current_thread != NULL)
				current_thread->thread_state = RUNNING;
		} 
	}
	else //control is here because current thread is NULL
	{
		current_thread = scheduler_get_next_thread();
		if (current_thread != NULL)
		{
			current_thread->thread_state = RUNNING;
		} 
	}

	//Update MLFQ on the basis of starvation
	starvation_counter++;
	if (starvation_counter > STARVATION_THRESHOLD - 1)
	{
		starvation_counter = 0;
		int i;
		for(i = 0; i < LEVELS; i++)
		{
			if((mlfq+i)->front != NULL)
			{
				my_pthread_t *curr_thread, *prev_thread;
				curr_thread = (mlfq+i)->front;
				prev_thread = NULL;
				while (curr_thread != NULL)
				{
					if ((time_stamp() - curr_thread->last_exec_time) > (QUANTUM * STARVATION_THRESHOLD))
					{
						if(prev_thread == NULL)
							(mlfq+i)->front = curr_thread->next;
						else
							prev_thread = curr_thread->next;

						scheduler_add_thread(curr_thread, 0); //add extracted thread to highest priority queue

					}
					else
						prev_thread = curr_thread;

					curr_thread = curr_thread->next;
				}
			}
		}
	}
	

	
    my_timer.it_value.tv_sec = 0;
    //my_timer.it_value.tv_usec = 50000;
    my_timer.it_value.tv_usec = QUANTUM;
    my_timer.it_interval.tv_sec = 0;
    my_timer.it_interval.tv_usec = 0;

    //set timer
    setitimer(ITIMER_REAL, &my_timer, NULL);

    //if(temp != NULL){
    	//getcontext(&uc_temp);
    	//temp->uc = uc_temp;
	//}
    
    if (current_thread != NULL) //Context switch happens here
    {
    	current_thread->last_exec_time = time_stamp();
    	if (temp != NULL)
    		swapcontext(&(temp->uc), &(current_thread->uc));
    	else
    		/*
    		stores main context
    		(enters else statement only in first call to scheduler)
    		*/
    		swapcontext(&uc_temp, &(current_thread->uc)); 
    }
    //return;
}

void scheduler_init() 
{	
	mlfq = malloc(LEVELS * sizeof(queue));
	if (mlfq == NULL)
		printf("error: malloc for mlfq failed\n");
	//printf("memory allocated for mlfq object\n");

	wait = malloc(sizeof(queue));
	if (wait == NULL)
		printf("error: malloc for wait queue failed\n");
	//printf("memory allocated for wait queue object\n");

	main_thread = malloc(sizeof(my_pthread_t));
	if (main_thread == NULL)
		printf("error: malloc for main_thread failed\n");
	//printf("memory allocated for main_thread\n");

	main_thread->id = 0;
	main_thread->next = main_thread;
	number_of_threads = 0;
	current_thread = NULL;
	//if (current_thread == NULL)
	//	printf("hola from init\n");
	int i;
	for(i = 0; i < LEVELS; i++)
	{
		queue_init(mlfq + i);
	}
	printf("mlfq initialized\n");
	queue_init(wait);
	printf("wait queue initialized\n");

	//signal(SIGVTALRM, scheduler);
	signal(SIGALRM, scheduler);
	printf("signal handler registered\n");
	scheduler();
}

void scheduler_add_thread(my_pthread_t *thread, int priority) 
{
	//printf("Adding thread %ld to level %d\n", thread->id, priority);
	thread->thread_state = READY;
	thread->running_time = 0; // initialization
	thread->priority = priority;
	enqueue(&(mlfq[priority]), thread);
	number_of_threads++;
}

my_pthread_t* scheduler_get_next_thread() 
{
	int i;
	for (i = 0; i < LEVELS; i++) 
	{
		if ((mlfq+i)->front != NULL) 
		{
			my_pthread_t *temp;
			temp = dequeue(mlfq+i); 
			// printf("Found a thread to schedule in level %d, thread id: %d\n", i, chosen->id);
			number_of_threads--;
			return temp;
		}
	}
	printf("no threads in scheduler\n");

	return NULL; //no threads in MLFQ
}

/*
	Wrapper function to execute the function associated with a thread. 
	Required to change current thread and thread states.
*/
void my_pthread_runner(my_pthread_t *thread, void *(*function)(void *), void *arg) 
{
	thread->thread_state = RUNNING;
	current_thread = thread;
	thread->return_val = function(arg);
	thread->thread_state = FINISHED;
	scheduler();
}

int my_pthread_create(my_pthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg) 
{
	/*
		The makecontext associates input thread to a wrapper function called my_pthread_runner. 
		my_pthread_runner is a wrapper used to execute the argument function
	*/

	//Run scheduler_init() if this is the first instance of my_pthread_t, first_thread is flag to check that
	if (first_thread == 0)
	{
		//getcontext(&(main_thread->uc));
		scheduler_init();
		printf("scheduler initialized\n");
		first_thread = 1;
	}

	thread->id = thread_counter++;
	if (getcontext(&(thread->uc)) != 0) 
	{
		printf("error: getting context failed\n");
		exit(1);
	}
	
	

	thread->uc.uc_link = NULL;
	thread->uc.uc_stack.ss_sp = malloc(MEM);
	if (thread->uc.uc_stack.ss_sp == 0)
	{
		printf("error: malloc for ucontext stack failed\n");
		exit(1);
	}
	thread->uc.uc_stack.ss_size = MEM;
	thread->uc.uc_stack.ss_flags = 0;

	thread->return_val = NULL;

	//makecontext(&(thread->uc), (void *)&function, 1, arg);
	// if (number_of_threads == 0)
	// {
	// 	current_thread = thread;
	// }
	
	makecontext(&(thread->uc), (void *)my_pthread_runner, 3, thread, function, arg);
	//makecontext(&(thread->uc), (void *)function, 1, arg);
	scheduler_add_thread(thread,0);
	printf("thread with id: %ld added to scheduler\n", thread->id);

	return 0;
}

void my_pthread_yield() 
{
	// int current_priority = current_thread->priority;
	// my_pthread_t * temp;
	// temp = current_thread;
	// scheduler_add_thread(temp, current_priority);
	// current_thread = scheduler_get_next_thread();
	// if (current_thread != NULL)
	// 	current_thread->thread_state = RUNNING;
    current_thread->thread_state = YIELDED;
	scheduler();
}

void my_pthread_exit(void *value_ptr) 
{
	/*
		Exits current_thread. 
		Save the return value from value_ptr in current_thread->return_val.
		If my_pthread_exit called, then the function did not return anything.
		Either an exit is called or the return executes successfully.
	*/
	
	if (current_thread->thread_state != FINISHED) 
	{
		current_thread->thread_state = FINISHED;
		current_thread->return_val = value_ptr;
		// value_ptr = current_thread->return_val;
		scheduler();
	}
}

int my_pthread_join(my_pthread_t *thread, void ** value_ptr) 
{
	/*
		Yield current thread unless referenced thread is finished.
		Save the return value from value_ptr in current_thread->return_val.
	*/
	while (thread->thread_state != FINISHED) {
		my_pthread_yield();
	}
	thread->return_val = value_ptr;
}

int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
    if (mutex == NULL)
    {
    	printf("error: mutex pointer to null\n");
        return 1;
    }

    mutex->lock  = 0;
    mutex->owner = NULL;
    mutex->wait = malloc(sizeof(queue));
    queue_init(mutex->wait);

    return 0;
}

int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) 
{
	/*
		__sync_val_compare_and_swap (type *ptr, type oldval, type newval) != 0) performs an atomic compare and swap. 
		That is, if the current value of *ptr is oldval, then write newval into *ptr.
		If the comparison is successful and newval was written, return contents of *ptr before the operation. 
	*/
    while (__sync_val_compare_and_swap (&(mutex->lock), 0, 1) != 0) //spin while lock not obtained
	{
		current_thread->thread_state = WAITING;
		printf("current thread has to wait for mutex, enqueue-ing to wait queue\n");
		enqueue(mutex->wait, current_thread);
		scheduler();
	}
	mutex->owner = current_thread;
}

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex)
{
    my_pthread_t *temp;
    if (mutex->wait->front != NULL) 
    {
		temp = dequeue(mutex->wait); 
		printf("dequeue-ing from wait queue and enqueue-ing to mlfq\n");
		scheduler_add_thread(temp, temp->priority);
	}
    mutex->lock = 0;
    printf("mutex is now available\n");
}

int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex)
{
    if (mutex == NULL)
    {
    	printf("error: mutex pointer to null\n");
    	return 1;
    }
    if (mutex->lock != 0)
    {
    	printf("error : mutex is locked\n");
        return 1;
    }
    free(mutex);
    
    return 0;
}


//TESTING
// void f0(int limit) 
// {
// 	long int i;
// 	while (i < limit * 100000) 
// 		i++;
// 	printf("i*100000: %d\n", i);
// }
void f0(int arr[])
{
   printf("insertion sort started\n");
   int i, key, j, n;
   n = sizeof(arr)/sizeof(int);
   for (i = 1; i < n; i++)
   {
       key = arr[i];
       j = i-1;
 
       while (j >= 0 && arr[j] > key)
       {
           arr[j+1] = arr[j];
           j = j-1;
       }
       arr[j+1] = key;
   }
   printf("largest element: %d\n", arr[n-1]);
}

void f1_with_mutex() 
{
	printf("Function Entry : f1_with_mutex\n");
    int i=0;
    int local_var;    

    my_pthread_mutex_lock(m);
    local_var = var_w_mutex;
    printf("f1_with_mutex: reading var_w_mutex, the value is %d\n", local_var);
    
    while(i<123456789)
    	i++;
    
    local_var = local_var + 100;
    var_w_mutex = local_var;
    printf("f1_with_mutex: writing var_w_mutex, the value now is %d\n", var_w_mutex);
    my_pthread_mutex_unlock(m);

    printf("Function Exit : f1_with_mutex\n");
}

void f2_with_mutex() 
{
	printf("Function Entry : f2_with_mutex\n");
    long int i=0;  
    int local_var;    

    my_pthread_mutex_lock(m);
    local_var = var_w_mutex;
    printf("f2_with_mutex: reading var_w_mutex, the value is %d\n", local_var);
    
    while(i<123456789)
    	i++;
    
    local_var = local_var - 50;
    var_w_mutex = local_var;
    printf("f2_with_mutex: writing var_w_mutex, the value now is %d\n", var_w_mutex);
    my_pthread_mutex_unlock(m);
    
    value_ptr = var_w_mutex;
    my_pthread_mutex_destroy(m);
    printf("Function Exit : f2_with_mutex\n");
}

void f1_without_mutex() 
{
	printf("Function Entry : f1_without_mutex\n");
    long int i=0;
    int local_var;   
    local_var = var_wo_mutex;
    printf("f1_without_mutex: reading var_wo_mutex, the value is %d\n", local_var);
    
    while(i<123456789)
    	i++;
    
    local_var = local_var + 100;
    var_wo_mutex = local_var;
    printf("f1_without_mutex: writing var=1, the value now is %d\n", var_wo_mutex);
    printf("Function Exit : f1_without_mutex\n");
}

void f2_without_mutex() 
{  
	printf("Function Entry : f2_without_mutex\n");
    long int i=0;  
    int local_var;     
    local_var = var_wo_mutex;
    printf("f2_without_mutex: read the var_wo_mutex, the value is %d\n", local_var);
    
    while(i<123456789)
    	i++;
    
    local_var = local_var - 50;
    var_wo_mutex = local_var;
    printf("f2_without_mutex: writing var_wo_mutex, the value now is %d\n", var_wo_mutex);
    printf("Function Exit : f2_without_mutex\n");
}

void f3() 
{
	printf("Function Entry : f3\n"); 
    long int i=0;   
    FILE *fp;  
    fp=fopen("f0.dat", "w"); 
    printf("writing to a file\n");
    while(i<123456789)
    {
    	fprintf(fp, "%d", 1); //Very very slow operation, is evident as well. Finishes at the last.
    	i++;
    }

    fflush(fp);
    printf("writing to file complete\n");
    fclose(fp); 
	printf("Function Exit : f3\n");
}

void f4() 
{
	printf("Function Entry : f4\n");
	int i;
	for(i = 10; i < 30; i++) 
		printf("i: %d\n", i);
	//my_pthread_join(&threads[7], NULL);
	printf("Function Exit : f4\n");
}

void f5() 
{
	printf("Function Entry : f5\n");
	int i;
	for(i = 30; i < 50; i++) 
	{
		printf("i: %d\n", i);
		if (i == 39) 
		{
			value_ptr = 10;
			printf("exit called here\n");
			my_pthread_exit(NULL);
		}
	}
	printf("Function Exit : f5\n");
}

void f6() 
{
	printf("Function Entry : f6\n");
	int i;
	for(i = 50; i < 70; i++) 
	{
		printf("i: %d\n", i);
		if (i == 59 || i == 60) 
		{
			my_pthread_yield();
		}
	}
	printf("Function Exit : f6\n");
}

void f7() 
{
	printf("Function Entry : f7\n");
	int i;
	for(i = 70; i < 90; i++) 
	{
		printf("i: %d\n", i);
		if (i == 79) 
		{
			value_ptr = 10;
			//my_pthread_join(&threads[7],NULL); //Waiting for f2_with_mutex() to finish
			my_pthread_join(&threads[7],value_ptr);
		}
	}
	printf("value_ptr:%d\n", value_ptr);
	printf("Function Exit : f7\n");
}

int main() 
{
	threads = malloc(NUM_THREADS * sizeof(my_pthread_t));
	if (threads == NULL)
		printf("error: malloc for threads failed\n");
	
	m = malloc(sizeof(my_pthread_mutex_t));
	if (m == NULL)
		printf("error: malloc for mutex failed\n");

	int m_ret = my_pthread_mutex_init(m,NULL);
	if (!m_ret)
		printf("mutex initialized\n");

	int i;
	long int arr[1000000];

	for (i = 0; i < 1000000; i++) 
	{
		arr[i] = 1000000 - i;
		//printf("i %ld\n", arr[i]);
	}	

	for (i = 0; i < NUM_THREADS-9; i++) 
	{
		if (my_pthread_create(&threads[i], NULL, (void *(*)(void *))f0, (void *)arr) == 1)
			printf("error: Creating Thread %d\n", i);
	}
	if (my_pthread_create(&threads[NUM_THREADS-9], NULL, (void *(*)(void *))f1_with_mutex, NULL) == 1)
			printf("error: creating Thread %d\n", NUM_THREADS-9);
	if (my_pthread_create(&threads[NUM_THREADS-8], NULL, (void *(*)(void *))f2_with_mutex, NULL) == 1)
			printf("error: creating Thread %d\n", NUM_THREADS-8);
	if (my_pthread_create(&threads[NUM_THREADS-7], NULL, (void *(*)(void *))f1_without_mutex, NULL) == 1)
			printf("error: creating Thread %d\n", NUM_THREADS-7);
	if (my_pthread_create(&threads[NUM_THREADS-6], NULL, (void *(*)(void *))f2_without_mutex, NULL) == 1)
			printf("error: creating Thread %d\n", NUM_THREADS-6);
	if (my_pthread_create(&threads[NUM_THREADS-5], NULL, (void *(*)(void *))f3, NULL) == 1)
			printf("error: creating Thread %d\n", NUM_THREADS-5);
	if (my_pthread_create(&threads[NUM_THREADS-4], NULL, (void *(*)(void *))f4, NULL) == 1)
			printf("error: creating Thread %d\n", NUM_THREADS-4);
	if (my_pthread_create(&threads[NUM_THREADS-3], NULL, (void *(*)(void *))f5, NULL) == 1)
			printf("error: creating Thread %d\n", NUM_THREADS-3);
	if (my_pthread_create(&threads[NUM_THREADS-2], NULL, (void *(*)(void *))f6, NULL) == 1)
			printf("error: creating Thread %d\n", NUM_THREADS-2);
	if (my_pthread_create(&threads[NUM_THREADS-1], NULL, (void *(*)(void *))f7, NULL) == 1)
			printf("error: creating Thread %d\n", NUM_THREADS-1);
	while(1);
	// int i;
	// for(i = 0; i < NUM_THREADS; i++)
	// 	my_pthread_join(&threads[i], NULL);

	return 0;
}