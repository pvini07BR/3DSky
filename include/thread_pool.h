#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <3ds.h>

#define THREAD_POOL_SIZE 4
#define TASK_QUEUE_CAPACITY 64

typedef struct {
    void (*func)(void*);
    void *arg;
    LightEvent doneEvent;
    bool done;
} ThreadTask;

typedef struct {
    int index;
    ThreadTask* task;
} ThreadTaskHandle;

typedef struct {
    Thread threads[THREAD_POOL_SIZE];
    ThreadTask tasks[TASK_QUEUE_CAPACITY];
    LightEvent hasTasks;
    LightLock queueLock;
    int head, tail, count;
    bool stop;
} ThreadPool;

void thread_pool_init();
bool thread_pool_add_task(void (*func)(void*), void* arg, ThreadTaskHandle* outHandle);
void thread_pool_task_wait(ThreadTaskHandle* handle);
bool thread_pool_task_is_done(ThreadTaskHandle* handle);
void thread_pool_free();

#endif