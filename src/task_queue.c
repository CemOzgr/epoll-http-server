#include "../include/task_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

typedef struct node {
    Task task;
    struct node *next;
} Node;

struct TaskQueue {
    Signal *signal;
    mtx_t mutex;
    Node *front;
    Node *back;
};

TaskQueue *task_queue_create(Signal *signal) {
    TaskQueue *queue = malloc(sizeof(TaskQueue));
    queue->front = NULL;
    queue->back = NULL;
    queue->signal = signal;

    mtx_init(&queue->mutex, mtx_plain);

    return queue;
}

void task_queue_destroy(TaskQueue *queue) {
    Node *current = queue->front;
    Node *next;
    
    while(current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    mtx_destroy(&queue->mutex);

    free(queue);
}

void task_queue_enqueue(TaskQueue *queue, const Task *task) {
    Node *new_node = malloc(sizeof(Node));

    if (new_node == NULL) {
        fprintf(stderr, "could not allocate memory for new node");
        exit(EXIT_FAILURE);
    }

    new_node->task = *task;
    new_node->next = NULL;

    mtx_lock(&(queue->mutex));

    if (queue->front == NULL) {
        queue->front = new_node;
        queue->back = new_node;
    } else {
        queue->back->next = new_node;
        queue->back = new_node;
    }

    mtx_unlock(&(queue->mutex));

    signal_block(queue->signal);
    signal(queue->signal);
    signal_unblock(queue->signal);
}

int task_queue_dequeue(TaskQueue *queue, Task *result) {
    mtx_lock(&(queue->mutex));

    if (queue->front == NULL) return -1;

    *result = queue->front->task;

    Node *node_to_delete = queue->front;
    queue->front = queue->front->next;

    if (queue->front == NULL) queue->back = NULL;

    free(node_to_delete);

    mtx_unlock(&(queue->mutex));

    return 1;
}

int task_queue_is_empty(TaskQueue *queue) {
    mtx_lock(&queue->mutex);

    int is_empty = queue->front == NULL;

    mtx_unlock(&queue->mutex);

    return is_empty;
}