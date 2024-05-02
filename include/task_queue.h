#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include "signaling.h"
typedef struct {
    int (*function)(void*);
    void *arg;
} Task;

typedef struct TaskQueue TaskQueue;

TaskQueue *task_queue_create(Signal *signal);
void task_queue_destroy(TaskQueue *queue);
void task_queue_enqueue(TaskQueue *queue, const Task *task);
int task_queue_dequeue(TaskQueue *queue, Task *result);
int task_queue_is_empty(TaskQueue *queue);

#endif