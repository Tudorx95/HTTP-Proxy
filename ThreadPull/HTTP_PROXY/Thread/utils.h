#ifndef THREAD_H
#define THREAD_H

#include "../Server/server.h"
#define MAX_THREADS 50
#define TASK_SIZE 100

typedef struct Task
{
    void (*task)(void *);
    void *arg;
} Task;

typedef struct ThreadPool
{
    pthread_t *threads;
    int thread_count;
    pthread_mutex_t mutex;
    pthread_cond_t cond; // condition related to thread tasks
    Task *tasks;
    int task_begin_index; // indexes for handling multiple threads accessing the resource
    int task_end_index;
    int task_count;
    short int task_available;
    short int state; // 0 = off; 1 = on
} ThreadPool;

extern ThreadPool *pool;

ThreadPool *thread_pool_init(int nb_threads);
void thread_pool_add_Task(ThreadPool *pool, void (*task)(void *), void *arg);
void thread_pool_exit(ThreadPool *pool);
void *thread_function(void *arg);

#endif