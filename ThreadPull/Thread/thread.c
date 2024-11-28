#include "utils.h"
#include "../utils.h"

ThreadPool *thread_pool_init(int nb_threads)
{
    int res;
    ThreadPool *thread_pool = malloc(sizeof(ThreadPool));
    if (!thread_pool)
    {
        perror("Error creating the ThreadPool!");
        return NULL;
    }
    res = pthread_mutex_init(&(thread_pool->mutex), NULL);
    if (res)
    {
        free(thread_pool);
        return NULL;
    }
    res = pthread_cond_init(&(thread_pool->cond), NULL);
    if (res)
    {
        pthread_mutex_destroy(&(thread_pool->mutex));
        free(thread_pool);
        return NULL;
    }

    // initiate Task vector
    thread_pool->task_count = thread_pool->task_begin_index = thread_pool->task_end_index = thread_pool->task_available = 0;
    thread_pool->tasks = malloc(TASK_SIZE * sizeof(Task));
    DIE(!(res = (thread_pool->tasks ? 1 : 0)), "Error allocating the tasks vector");
    if (!res)
    {
        free(thread_pool->tasks);
        free(thread_pool);
        return NULL;
    }
    thread_pool->state = 1; // state on by default

    // create threads
    thread_pool->thread_count = 0;
    thread_pool->threads = (pthread_t *)malloc(nb_threads * sizeof(pthread_t));
    DIE(!(res = (thread_pool->threads ? 1 : 0)), "Error allocating the threads");
    if (!res)
    {
        free(thread_pool->tasks);
        free(thread_pool->threads);
        free(thread_pool);
        return NULL;
    }

    for (int i = 0; i < nb_threads; i++)
    {
        DIE((res = pthread_create(&(thread_pool->threads[i]), NULL, thread_function,
                                  (void *)thread_pool)) != 0,
            "Error creating thread");
        if (res != 0)
        {
            thread_pool_exit(thread_pool);
            return NULL;
        }
    }

    return thread_pool;
}

void *thread_function(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    Task newtask;

    while (1)
    {
        pthread_mutex_lock(&(pool->mutex));
        // task_count -> fair count     !!!
        // task_available -> for wait condition     !!!
        while (pool->task_available == 0 && (pool->state))
        {
            pthread_cond_wait(&(pool->cond), &(pool->mutex));
        }
        if (!(pool->state))
        {
            pthread_mutex_unlock(&(pool->mutex));
            pthread_exit(NULL);
        }

        // fetch the task functions
        pool->task_available--;
        newtask.arg = pool->tasks[pool->task_begin_index].arg;
        newtask.task = pool->tasks[pool->task_begin_index].task;
        // pool->task_begin_index++;
        pool->task_begin_index = (pool->task_begin_index + 1) % TASK_SIZE;

        printf("Thread_func: Task begin: %d Task_count: %d for thread_id: %ld\n", pool->task_begin_index,
               pool->task_count, pthread_self());

        // signaling the condition
        pthread_mutex_unlock(&(pool->mutex));

        (*(newtask.task))(newtask.arg);
    }
    pthread_exit(NULL);
}

void thread_pool_add_Task(ThreadPool *pool, void (*task)(void *), void *arg)
{
    pthread_mutex_lock(&(pool->mutex));
    // if the threadPool state is off or there are too many tasks
    if (!(pool->state))
    {
        pthread_mutex_unlock(&(pool->mutex));
        return;
    }

    // identify the next task
    pool->tasks[pool->task_count].arg = arg;
    pool->tasks[pool->task_count].task = task;
    // pool->task_count++; // increase the current last task
    //  pool->task_end_index++;
    pool->task_count = (pool->task_count + 1) % TASK_SIZE;
    printf("Add_Task: Task end_index: %d Task count: %d for thread_id: %ld\n", pool->task_end_index,
           pool->task_count, pthread_self());

    pthread_cond_broadcast(&(pool->cond));
    pthread_mutex_unlock(&(pool->mutex));
}

void thread_pool_exit(ThreadPool *pool)
{
    pthread_mutex_lock(&(pool->mutex));
    pool->state = 0; // set off state

    // if (pool->task_count > 0)
    {
        // Signal all the waiting threads to stop
        pthread_cond_broadcast(&(pool->cond));
    }

    pthread_mutex_unlock(&(pool->mutex));

    for (int i = 0; i < pool->thread_count; i++)
        pthread_detach(pool->threads[i]);

    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->cond));

    free(pool->threads);
    free(pool->tasks);
    free(pool);
}