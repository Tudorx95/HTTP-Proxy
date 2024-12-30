#include "./utils.h"
#include "../History/utils.h"

void signal_caught(int sig)
{
    Free_Logging(logger);
    switch (sig)
    {
    case SIGHUP:
        printf("Signal %d caught!", sig);
        break;
    case SIGINT:
        printf("Signal %d caught!", sig);
        break;
    case SIGTERM:
        printf("Signal %d caught!", sig);
        break;
    case SIGKILL:
        printf("Signal %d caught!", sig);
        break;
    case SIGILL:
        printf("Signal %d caught!", sig);
        break;
    case SIGCONT:
        printf("Signal %d caught!", sig);
        break;
    case SIGSTOP:
        printf("Signal %d caught!", sig);
        break;
    case SIGTTIN:
        printf("Signal %d caught!", sig);
        break;
    case SIGTTOU:
        printf("Signal %d caught!", sig);
        break;
    default:
        printf("Signal %d caught!", sig);
        break;
    }
}