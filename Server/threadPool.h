#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

// Definirea dimensiunii maxime a cozii de task-uri
#define TASK_QUEUE_SIZE 64

// Structura pentru thread pool
typedef struct {
    pthread_t *threads;                        // Vector de thread-uri
    void (*task_queue[TASK_QUEUE_SIZE])(void *); // Coada de task-uri (funcții)
    void *task_args[TASK_QUEUE_SIZE];           // Argumentele task-urilor
    int queue_front, queue_rear;                // Indici pentru coadă
    int shutdown;                               // Flag pentru oprirea pool-ului

    pthread_mutex_t queue_mutex;                // Mutex pentru sincronizare
    pthread_cond_t queue_cond;                  // Condiție pentru notificare
    int thread_count;                           // Numărul de thread-uri
} ThreadPool;

// Funcții pentru gestionarea thread pool-ului
ThreadPool *createThreadPool(int thread_count);
int addTaskToPool(ThreadPool *pool, void (*task)(void *), void *arg);
void destroyThreadPool(ThreadPool *pool);

#endif // THREADPOOL_H
