#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

#define TASK_QUEUE_SIZE 10


typedef struct ThreadPool {
    pthread_t *threads;                  
    void (*task_queue[TASK_QUEUE_SIZE])(void *); 
    void *task_args[TASK_QUEUE_SIZE];    
    int queue_front;                    
    int queue_rear;                      
    int shutdown;                        
    int thread_count;                    
    pthread_mutex_t queue_mutex;         
    pthread_cond_t queue_cond;           
} ThreadPool;


ThreadPool *createThreadPool(int thread_count);
int addTaskToPool(ThreadPool *pool, void (*task)(void *), void *arg);
void destroyThreadPool(ThreadPool *pool);

#endif // THREAD_POOL_H
