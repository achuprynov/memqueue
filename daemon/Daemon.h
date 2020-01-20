/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#pragma once

#include <functional>

class Daemon
{
private:
    Daemon() {}

public:
    static void daemonize
    (
        std::function<void()> && sigtermHandler = []{exit(EXIT_SUCCESS);}
    );

private:
    static int  set_error_handlers();
    static void set_signal_handler();
    static void handle_signal(int signum);
};
