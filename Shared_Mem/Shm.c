#include "utils.h"
#include "../utils.h"
#include "../Cache/utils.h"

sem_t *sem = NULL;
int shm_fd;
int listener_pipe[2];
int forward_pipe[2];

void create_SHM()
{
    // Define the shared memory object
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error creating the shared memory object");
        exit(EXIT_FAILURE);
    }
    // Set the size of the shared memory object to fit the Cache structure
    if (ftruncate(shm_fd, sizeof(CacheManagementUnit)) == -1)
    {
        perror("Error ftruncate the cache");
        exit(EXIT_FAILURE);
    }
    // Map the shared memory into the process's address space
    cache = mmap(NULL, sizeof(CacheManagementUnit), PROT_READ | PROT_EXEC, MAP_SHARED, shm_fd, 0);
    if (cache == MAP_FAILED)
    {
        perror("mmap failed");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    // Initialize shared memory cache structure
    cache = create_cache();
    // Create or open a named semaphore for synchronizing access
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED)
    {
        perror("Error initializing semaphore");
        munmap(cache, sizeof(CacheManagementUnit));
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
}