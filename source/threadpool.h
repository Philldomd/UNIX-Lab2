#pragma once

typedef struct threadpool_t threadpool_t;

enum threadpool_error_t
{
	threadpool_invalid 			= -1,
	threadpool_lock_failure 	= -2,
	threadpool_queue_full 		= -3,
	threadpool_shutdown 		= -4,
	threadpool_thread_failure 	= -5,
};

enum threadpool_destroy_flag_t
{
	threadpool_graceful 		= 1,
};

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags);
int threadpool_add(threadpool_t *pool, void (*function) (void*), void *arg);
int threadpool_destroy(threadpool_t *pool, int flags);
