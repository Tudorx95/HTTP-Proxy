#ifndef HIST_UTILS_H
#define HIST_UTILS_H

#define LOG_FILE "log.txt"
#include <pthread.h>

typedef struct Logging
{
    pthread_mutex_t mutex;
    int fd;
} Logging;

extern Logging *logger;
Logging *set_LogStruct();
void Free_Logging(Logging *lg);
void Log_AddMessage(Logging *lg, const char *buffer);

#endif