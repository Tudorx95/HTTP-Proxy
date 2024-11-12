#include "./Server/server.h"
#include "./Cache/utils.h"
#include "./Signal_Handlers/utils.h"
#include "utils.h"
#include "./Shared_Mem/utils.h"
#include "./TerminalGUI/utils.h"
#include <pthread.h>

// divide processes
void delegateTasks()
{
    // set Cache as shared memory object
    create_SHM();
    // create a process for user interaction with the terminal
    runConnection();
}

int main()
{
    delegateTasks();
    return 0;
}
