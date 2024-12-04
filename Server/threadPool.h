#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>

typedef struct ThreadPool {
    pthread_t *threads;
    int thread_count;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    void (**task_queue)(void *);
    void **task_args;
    int queue_size;
    int queue_front;
    int queue_rear;
    int shutdown;
} ThreadPool;

ThreadPool *createThreadPool(int thread_count, int queue_size);
void destroyThreadPool(ThreadPool *pool);
int addTaskToPool(ThreadPool *pool, void (*task)(void *), void *arg);

#endif // THREAD_POOL_H
