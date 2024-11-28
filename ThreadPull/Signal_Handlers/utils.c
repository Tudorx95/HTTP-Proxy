#include "./utils.h"
#include "../utils.h"
#include "../Server/server.h"
int initiate_SigHandler()
{
    // set the sigaction to encounter SIGINT states
    struct sigaction sig_act;
    memset(&sig_act, 0, sizeof(sig_act));
    sig_act.sa_flags = SA_RESETHAND;
    sig_act.sa_handler = signal_caught;
    if (sigaction(SIGINT, &sig_act, NULL))
    {
        perror("Error setting the signal handler for SIGINT");
        return -1;
    }

    sigset_t set;
    int fd;
    // clear signals from set var
    DIE((fd = sigemptyset(&set)) == -1, "Error clearing the signal mask");
    if (fd == -1)
        return -1;
    // add the SIG INTERRUPT signal
    sigaddset(&set, SIGINT);
    // create a new fd specific for handling SPECIFIC SIGINT signals
    int sigfd;
    DIE((sigfd = signalfd(-1, &set, 0)) == -1, "Error creating the signal fd");
    return sigfd;
}

void signal_caught(int sig)
{
    // more info in man signalfd
    struct signalfd_siginfo fdsi;
    int readBytes;
    DIE((readBytes = read(sig, &fdsi, sizeof(fdsi))) == -1, "Error reading bytes for sigfd");
    if (readBytes == -1)
        exit(EXIT_FAILURE);

    switch ((int)fdsi.ssi_signo)
    {
    case SIGHUP:
        printf("Signal %d caught!", sig);
        break;
    case SIGINT:
        printf("Signal %d caught!", sig);
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
        break;
    }
}