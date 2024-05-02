#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>
#include "../include/signaling.h"
#include "../include/thread_pool.h"
#include "../include/task_queue.h"

typedef enum { IDLE, RUNNING, STOPPED } ThreadState;

typedef struct {
    int index;
    thrd_t thread_id;
    ThreadState state;
    Task task; // remove this and put task_queue instead
} WorkerThread;

struct ThreadPool {
    int size;
    Signal *signal;
    TaskQueue *queue;
    WorkerThread threads[];
};

WorkerThread *find_by_id(ThreadPool *pool, thrd_t id) {
    for (int i=0; i<pool->size; i++) {
        if (pool->threads[i].thread_id == id) {
            return &(pool->threads[i]);
        }
    }

    return NULL;
}

int start_worker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;

    WorkerThread *current = find_by_id(pool, thrd_current());
    Task *task = &(current->task);

    printf("Thread %d: initialized\n", current->index);
    
    for(;;) {
        signal_block(pool->signal);

        while(task_queue_is_empty(pool->queue) && current->state == RUNNING) {
            signal_wait(pool->signal);
        }

        if (current->state == STOPPED) {
            signal_unblock(pool->signal);
            break;
        }

        Task task;
        int result = task_queue_dequeue(pool->queue, &task);

        signal_unblock(pool->signal);
        
        if (result != -1) task.function(task.arg);
    }

    return 0;
}

int execute_from_queue(void  *arg) {
    TaskQueue *queue = (TaskQueue *)arg;

    Task task;
    int result = task_queue_dequeue(queue, &task);

    if (result == -1) return -1;

    task.function(task.arg);

    return 0;
}


ThreadPool *thread_pool_initialize(int size, TaskQueue *queue, Signal *signal) {
    ThreadPool *pool = malloc(sizeof(ThreadPool) + sizeof(WorkerThread) * size);
    pool->size = size;
    pool->queue = queue;
    pool->signal = signal;

    Task task = {
        .function = execute_from_queue,
        .arg = queue
    };

    for (int i=0; i<size; i++) {
        thrd_t t;
        thrd_create(&t, start_worker, pool);

        pool->threads[i].index = i;
        pool->threads[i].task = task;
        pool->threads[i].thread_id = t;
        pool->threads[i].state = RUNNING;
    }

    return pool;
}

void thread_pool_destroy(ThreadPool *pool) {
    signal_block(pool->signal);

    for (int i = 0; i<pool->size; i++) {
        pool->threads[i].state = STOPPED;
    }

    signal_broadcast(pool->signal);

    signal_unblock(pool->signal);

    for (int i = 0; i<pool->size; i++) {
        thrd_join(pool->threads[i].thread_id, NULL);
    }

    free(pool);
}