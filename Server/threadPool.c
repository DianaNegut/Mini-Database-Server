#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



void *threadWorker(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;

    while (1)
    {
        void (*task)(void *) = NULL;
        void *task_arg = NULL;

        pthread_mutex_lock(&pool->queue_mutex);

        while (pool->queue_front == pool->queue_rear && !pool->shutdown)
        {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }

        if (pool->shutdown)
        {
            printf("Thread %lu: Pool-ul se închide.\n", pthread_self());
            pthread_mutex_unlock(&pool->queue_mutex);
            pthread_exit(NULL);
        }

        task = pool->task_queue[pool->queue_front];
        task_arg = pool->task_args[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % TASK_QUEUE_SIZE;

        printf("Thread %lu: A preluat un task din coadă.\n", pthread_self());

        pthread_mutex_unlock(&pool->queue_mutex);



        task(task_arg);
        printf("Thread %lu: A finalizat task-ul.\n", pthread_self());
    }
}



ThreadPool *createThreadPool(int thread_count)
{
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (!pool)
    {
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

    printf("Se creează un thread pool cu %d thread-uri.\n", thread_count);

    for (int i = 0; i < thread_count; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, threadWorker, pool) != 0)
        {
            perror("Eroare la crearea unui thread");
            free(pool->threads);
            free(pool);
            return NULL;
        }
        printf("Thread %d (ID %lu) creat cu succes.\n", i + 1, pool->threads[i]);
    }

    return pool;
}


int addTaskToPool(ThreadPool *pool, void (*task)(void *), void *arg)
{
    pthread_mutex_lock(&pool->queue_mutex);

    int next_rear = (pool->queue_rear + 1) % TASK_QUEUE_SIZE;
    if (next_rear == pool->queue_front)
    {

        pthread_mutex_unlock(&pool->queue_mutex);
        printf("Task respins: Thread pool-ul este plin! Toate sloturile sunt ocupate.\n");
        return -1;
    }


    pool->task_queue[pool->queue_rear] = task;
    pool->task_args[pool->queue_rear] = arg;
    pool->queue_rear = next_rear;

    int tasks_in_queue = (pool->queue_rear >= pool->queue_front)
                            ? (pool->queue_rear - pool->queue_front)
                            : (TASK_QUEUE_SIZE - pool->queue_front + pool->queue_rear);

    printf("Task adăugat: %d task-uri în coadă acum.\n", tasks_in_queue);

    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    return 0;
}


void destroyThreadPool(ThreadPool *pool)
{
    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    printf("Thread pool-ul se închide...\n");

    for (int i = 0; i < pool->thread_count; i++)
    {
        pthread_join(pool->threads[i], NULL);
        printf("Thread %d (ID %lu) a fost închis.\n", i + 1, pool->threads[i]);
    }

    free(pool->threads);
    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_cond);
    free(pool);

    printf("Thread pool-ul a fost distrus.\n");
}
