/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "Daemon.h"

std::function<void()> sigtermHandler;

void Daemon::daemonize(std::function<void()> && _sigtermHandler)
{
    int ret_code = 0;
    sigtermHandler = _sigtermHandler;
    
    auto pid = fork();

    if (pid == -1)
    {
        exit(EXIT_FAILURE);
    }
    else if (pid)
    {
        exit(EXIT_SUCCESS);
    }

    umask(0);
    setsid();
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    auto fd = open("/dev/null", O_RDWR);
    if (fd)
    {
        exit(EXIT_FAILURE);
    }
    
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    
    ret_code = set_error_handlers();
    if (ret_code != 0)
        exit(ret_code);

    set_signal_handler();
}

int Daemon::set_error_handlers()
{
    int ret_code = 0;
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(sigact));

    sigact.sa_flags = SA_SIGINFO;

    ret_code = sigemptyset(&sigact.sa_mask);
    if (ret_code != 0)
        return ret_code;

    ret_code = sigaction(SIGFPE,  &sigact, 0);
    if (ret_code != 0)
        return ret_code;

    ret_code = sigaction(SIGILL,  &sigact, 0);
    if (ret_code != 0)
        return ret_code;

    ret_code = sigaction(SIGSEGV, &sigact, 0);
    if (ret_code != 0)
        return ret_code;

    ret_code = sigaction(SIGBUS,  &sigact, 0);
    if (ret_code != 0)
        return ret_code;

    return 0;
}

void Daemon::set_signal_handler()
{
    for (auto signo = 1; signo < _NSIG; signo++)
    {
        if (signo == SIGFPE  || 
            signo == SIGILL  || 
            signo == SIGSEGV || 
            signo == SIGBUS  ||
            signo == SIGKILL ||
            signo == SIGSTOP ||
            signo == SIGCONT)
        {
            continue;
        }
        
        signal(signo, handle_signal);
    }
}

void Daemon::handle_signal(int signum)
{
    if (signum == SIGTERM)
    {
        sigtermHandler();
    }
}
