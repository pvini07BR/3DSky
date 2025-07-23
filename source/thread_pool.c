#include "thread_pool.h"
#include "3ds/svc.h"

static ThreadPool threadPool;

void thread_pool_worker(void* arg);

void thread_pool_init() {
    threadPool.head = threadPool.tail = threadPool.count = 0;
    threadPool.stop = false;

    LightLock_Init(&threadPool.queueLock);
    LightEvent_Init(&threadPool.hasTasks, RESET_ONESHOT);

    s32 prio;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        threadPool.threads[i] = threadCreate(thread_pool_worker, &threadPool, 16 * 1024, prio-1, -2, false);
    }
}

bool thread_pool_add_task(void (*func)(void*), void* arg, ThreadTaskHandle* outHandle) {
    LightLock_Lock(&threadPool.queueLock);
    if (threadPool.count >= TASK_QUEUE_CAPACITY) {
        LightLock_Unlock(&threadPool.queueLock);
        return false;
    }

    int idx = threadPool.tail;
    ThreadTask* task = &threadPool.tasks[idx];
    task->func = func;
    task->arg = arg;
    task->done = false;
    LightEvent_Init(&task->doneEvent, RESET_ONESHOT);

    if (outHandle) {
        outHandle->index = idx;
        outHandle->task = task;
    }

    threadPool.tail = (threadPool.tail + 1) % TASK_QUEUE_CAPACITY;
    threadPool.count++;
    LightEvent_Signal(&threadPool.hasTasks);
    LightLock_Unlock(&threadPool.queueLock);
    return true;
}

void thread_pool_worker(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;

    while (true) {
        LightEvent_Wait(&pool->hasTasks);

        if (pool->stop) break;

        ThreadTask task;

        LightLock_Lock(&pool->queueLock);
        if (pool->count > 0) {
            int idx = pool->head;
            task = pool->tasks[idx];
            pool->head = (pool->head + 1) % TASK_QUEUE_CAPACITY;
            pool->count--;

            if (pool->count > 0) {
                LightEvent_Signal(&pool->hasTasks);
            }

            LightLock_Unlock(&pool->queueLock);

            if (task.func) {
                task.func(task.arg);
            }

            // Sinaliza que a tarefa terminou
            LightLock_Lock(&pool->queueLock);
            pool->tasks[idx].done = true;
            LightEvent_Signal(&pool->tasks[idx].doneEvent);
            LightLock_Unlock(&pool->queueLock);
        } else {
            LightLock_Unlock(&pool->queueLock);
        }
    }
}

void thread_pool_task_wait(ThreadTaskHandle* handle) {
    if (!handle || !handle->task) return;
    LightEvent_Wait(&handle->task->doneEvent);
}

bool thread_pool_task_is_done(ThreadTaskHandle* handle) {
    if (!handle || !handle->task) return false;
    return handle->task->done;
}

void thread_pool_free() {
    threadPool.stop = true;

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        LightEvent_Signal(&threadPool.hasTasks);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        threadJoin(threadPool.threads[i], U64_MAX);
        threadFree(threadPool.threads[i]);
    }

    LightEvent_Clear(&threadPool.hasTasks);
}