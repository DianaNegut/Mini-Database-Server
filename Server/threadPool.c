#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *threadWorker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;

    while (1) {
        void (*task)(void *) = NULL;
        void *task_arg = NULL;


        pthread_mutex_lock(&pool->queue_mutex);

        while (pool->queue_front == pool->queue_rear && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }


        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->queue_mutex);
            pthread_exit(NULL);
        }


        task = pool->task_queue[pool->queue_front];
        task_arg = pool->task_args[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % TASK_QUEUE_SIZE;

    
        pthread_mutex_unlock(&pool->queue_mutex);


        task(task_arg);
    }
}


ThreadPool *createThreadPool(int thread_count) {
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (!pool) {
        perror("Eroare la alocarea memoriei pentru thread pool");
        return NULL;
    }

    pool->threads = (pthread_t *)malloc(thread_count * sizeof(pthread_t));
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->shutdown = 0;
    pool->thread_count = thread_count;

    pthread_mutex_init(&pool->queue_mutex, NULL);
    pthread_cond_init(&pool->queue_cond, NULL);


    for (int i = 0; i < thread_count; i++) {
        pthread_create(&pool->threads[i], NULL, threadWorker, pool);
    }

    return pool;
}


int addTaskToPool(ThreadPool *pool, void (*task)(void *), void *arg) {
    pthread_mutex_lock(&pool->queue_mutex);

    int next_rear = (pool->queue_rear + 1) % TASK_QUEUE_SIZE;
    if (next_rear == pool->queue_front) {
        pthread_mutex_unlock(&pool->queue_mutex);
        return -1; 
    }

    pool->task_queue[pool->queue_rear] = task;
    pool->task_args[pool->queue_rear] = arg;
    pool->queue_rear = next_rear;

    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    return 0;
}


void destroyThreadPool(ThreadPool *pool) {
    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);
    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_cond);
    free(pool);
}
