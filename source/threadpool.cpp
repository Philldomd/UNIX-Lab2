#include "threadpool.h" 

#include <cstdlib>
#include <pthread.h>
#include <unistd.h>

enum threadpool_shutdown_t
{
	immediate_shutdown = 1,
	graceful_shutdown = 2,
};

struct threadpool_task_t
{
	void (*function)(void *);
	void *argument;
};

struct threadpool_t {
	pthread_mutex_t lock;
	pthread_cond_t notify;
	pthread_t *threads;
	threadpool_task_t *queue;
	int thread_count;
	int queue_size;
	int head;
	int tail;
	int count;
	int shutdown;
	int started;
};

static void *threadpool_thread(void *threadpool);
int threadpool_free(threadpool_t *pool);
int threadpool_join(threadpool_t *pool, int flags);
int threadpool_add_task(threadpool_t *pool, void (*funtion)(void *), void *arg);

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags)
{
	threadpool_t *pool;
	int i;
	
	if((pool = (threadpool_t*)malloc(sizeof(threadpool_t))) == NULL)
	{
		if(pool)
		{
			threadpool_free(pool);
		}
		return NULL;
	}
	
	pool->thread_count = 0;
	pool->queue_size = queue_size;
	pool->head = pool->tail = pool->count = 0;
	pool->shutdown = pool->started = 0;
	
	pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
	pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);
	
	if((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
		(pthread_cond_init(&(pool->notify), NULL) != 0) ||
		(pool->threads == NULL) ||
		(pool->queue == NULL))
	{
		if(pool)
		{
			threadpool_free(pool);
		}
		return NULL;
	}
	for(i = 0; i < thread_count; i++)
	{
		if(pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool) != 0)
		{
			threadpool_destroy(pool, 0);
			return NULL;
		}
		pool->thread_count++;
		pool->started++;
	}
	
	return pool;
}

int threadpool_add(threadpool_t *pool, void (*function)(void *), void *arg)
{
	int err = 0;
	
	
	if(pool == NULL || function || NULL)
	{
		return threadpool_invalid;
	}
	
	if(pthread_mutex_lock(&(pool->lock)) != 0)
	{
		return threadpool_lock_failure;
	}
	
	err = threadpool_add_task(pool, function, arg);
	
	if(pthread_mutex_unlock(&pool->lock) != 0)
	{
		err = threadpool_lock_failure;
	}
	
	return err;
}

int threadpool_add_task(threadpool_t *pool, void (*function)(void *), void *arg)
{
	int next = pool->tail +1;
	
	next = (next == pool->queue_size) ? 0 : next;
	
	if(pool->count == pool->queue_size)
	{
		return threadpool_queue_full;
	}
	
	if(pool->shutdown)
	{
		return threadpool_shutdown;
	}
	
	pool->queue[pool->tail].function = function;
	pool->queue[pool->tail].argument = arg;
	pool->tail = next;
	pool->count += 1;
	
	if(pthread_cond_signal(&(pool->notify)) != 0)
	{
		return threadpool_lock_failure;
	}
	return 0;	
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
	int err = 0;
	
	if(pool == NULL)
	{
		return threadpool_invalid;
	}
	
	if(pthread_mutex_lock(&(pool->lock)) !=0)
	{
		return threadpool_lock_failure;
	} 
	
	err = threadpool_join(pool, flags);
	
	if(!err)
	{
		threadpool_free(pool);
	}
	return err;
}

int threadpool_join(threadpool_t *pool, int flags)
{
	int i;
	
	if(pool->shutdown)
	{
		return threadpool_shutdown;
	}
	
	pool->shutdown = (flags & threadpool_graceful) ? 
			graceful_shutdown : immediate_shutdown;
			
	if((pthread_cond_broadcast(&(pool->notify)) != 0) ||
		(pthread_mutex_unlock(&(pool->lock)) != 0))
	{
		return threadpool_lock_failure;
	}
	
	for(i = 0; i < pool->thread_count; i++)
	{
		if(pthread_join(pool->threads[i],NULL) != 0)
		{
			return threadpool_thread_failure;
		}
	}
	return 0;
}

int threadpool_free(threadpool_t *pool)
{
	if(pool == NULL || pool->started  > 0)
	{
		return -1;
	}
	
	if(pool->threads)
	{
		free(pool->threads);
		free(pool->queue);
		
		pthread_mutex_lock(&(pool->lock));
		pthread_mutex_destroy(&(pool->lock));
		pthread_cond_destroy(&(pool->notify));
	}
	free(pool);
	return 0;
}

static void *threadpool_thread(void *threadpool)
{
	threadpool_t *pool = (threadpool_t *)threadpool;
	threadpool_task_t task;
	
	while(true)
	{
		pthread_mutex_lock(&(pool->lock));
		
		while((pool->count == 0) && (!pool->shutdown))
		{
			pthread_cond_wait(&(pool->notify), &(pool->lock));
		}
		
		if((pool->shutdown == immediate_shutdown) ||
			((pool->shutdown == graceful_shutdown) &&
			(pool->count == 0)))
		{
			break;
		}
		
		task.function = pool->queue[pool->head].function;
		task.argument = pool->queue[pool->head].argument;
		pool->head += 1;
		pool->head = (pool->head == pool->queue_size) ? 0 : pool->head;
		pool->count -= 1;
		
		pthread_mutex_unlock(&(pool->lock));
		
		(*(task.function))(task.argument);
	}
	
	pool->started--;
	
	pthread_mutex_unlock(&(pool->lock));
	pthread_exit(NULL);
	return(NULL);
}
