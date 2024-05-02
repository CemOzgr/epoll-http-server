#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "task_queue.h"

typedef struct ThreadPool ThreadPool;

ThreadPool *thread_pool_initialize(int size, TaskQueue *queue, Signal *signal);
void thread_pool_destroy(ThreadPool *pool);

#endif