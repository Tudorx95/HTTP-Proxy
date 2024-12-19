#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>

#define SHM_NAME "/Cache_SHM"
#define SEM_NAME "/Cache_Semaphore"

extern int shm_fd;
extern sem_t *sem;
extern int listener_pipe[2];
extern int forward_pipe[2];
void create_SHM();

#endif