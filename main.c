#include "./Server/server.h"
// #include "./Cache/utils.h"
#include "./Signal_Handlers/utils.h"
#include "utils.h"
#include "./History/utils.h"
#include <pthread.h>

// divide processes
void delegateTasks()
{
    // set the log structure
    logger = set_LogStruct();
    if (!logger)
    {
        DIE(1, "Error creating the Logger!");
        return;
    }
    // create a process for user interaction with the terminal
    runConnection();
    // liberate Logging struct
    Free_Logging(logger);
}

int main()
{
    delegateTasks();
    return 0;
}
