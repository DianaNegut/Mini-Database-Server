#include "threadPool.h"
#include <stdio.h>

void *threadWorker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;

    while (1) {
        pthread_mutex_lock(&pool->queue_mutex);

        while (pool->queue_front == pool->queue_rear && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->queue_mutex);
            pthread_exit(NULL);
        }

        void (*task)(void *) = pool->task_queue[pool->queue_front];
        void *task_arg = pool->task_args[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % pool->queue_size;

        pthread_mutex_unlock(&pool->queue_mutex);

        task(task_arg);
    }
}

ThreadPool *createThreadPool(int thread_count, int queue_size) {
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (!pool) {
        perror("Eroare la alocarea memoriei pentru thread pool");
        return NULL;
    }

    pool->threads = (pthread_t *)malloc(thread_count * sizeof(pthread_t));
    pool->task_queue = (void (**)(void *))malloc(queue_size * sizeof(void (*)(void *)));
    pool->task_args = (void **)malloc(queue_size * sizeof(void *));
    pool->thread_count = thread_count;
    pool->queue_size = queue_size;
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->shutdown = 0;

    pthread_mutex_init(&pool->queue_mutex, NULL);
    pthread_cond_init(&pool->queue_cond, NULL);

    for (int i = 0; i < thread_count; i++) {
        pthread_create(&pool->threads[i], NULL, threadWorker, pool);
    }

    return pool;
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
    free(pool->task_queue);
    free(pool->task_args);

    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_cond);

    free(pool);
}

int addTaskToPool(ThreadPool *pool, void (*task)(void *), void *arg) {
    pthread_mutex_lock(&pool->queue_mutex);

    int next_rear = (pool->queue_rear + 1) % pool->queue_size;
    if (next_rear == pool->queue_front) {
        pthread_mutex_unlock(&pool->queue_mutex);
        return -1; // Coada este plina
    }

    pool->task_queue[pool->queue_rear] = task;
    pool->task_args[pool->queue_rear] = arg;
    pool->queue_rear = next_rear;

    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    return 0;
}
