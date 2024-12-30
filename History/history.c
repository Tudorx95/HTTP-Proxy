#include "utils.h"
#include "../utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

Logging *logger = NULL;

Logging *set_LogStruct()
{
    Logging *lg = malloc(sizeof(Logging));
    if (!lg)
    {
        DIE(1, "Error allocating the Logging!");
        return NULL;
    }
    lg->fd = open(LOG_FILE, O_CREAT | O_APPEND | O_WRONLY, 0644);
    pthread_mutex_init(&lg->mutex, NULL);
    DIE(lg->fd == -1, "Error creating/opening logfile!");
    if (lg->fd == -1)
    {
        free(lg);
        return NULL;
    }
    return lg;
}

void Free_Logging(Logging *lg)
{
    if (lg->fd != -1)
        close(lg->fd);
    pthread_mutex_destroy(&lg->mutex);
    free(lg);
}

void Log_AddMessage(Logging *lg, const char *buffer)
{
    printf("Hello from logging!");
    if (!lg)
        return;
    pthread_mutex_lock(&lg->mutex);
    if (lg->fd == -1)
    {
        pthread_mutex_unlock(&lg->mutex);
        return;
    }
    // Get the current time
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    // Format the timestamp
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S] ", local_time);

    printf("%d\n", lg->fd);
    // Write the timestamp to the log file
    if (write(lg->fd, timestamp, strlen(timestamp)) == -1)
    {
        pthread_mutex_unlock(&lg->mutex);
        DIE(1, "Error writing timestamp to logFile!");
    }

    // Write the message to the log file
    if (write(lg->fd, buffer, strlen(buffer)) == -1)
    {
        pthread_mutex_unlock(&lg->mutex);
        DIE(1, "Error writing message to logFile!");
    }

    // Add a newline after the message
    const char *newline = "\n";
    if (write(lg->fd, newline, strlen(newline)) == -1)
    {
        pthread_mutex_unlock(&lg->mutex);
        DIE(1, "Error writing newline to logFile!");
    }

    pthread_mutex_unlock(&lg->mutex);
}
