#ifndef SIG_UTILS_H
#define SIG_UTILS_H

#include <stdio.h>
#include <signal.h>
#include <sys/signal.h>
#include <sys/signalfd.h>

int initiate_SigHandler();
void signal_caught(int sig);

#endif