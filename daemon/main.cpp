/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <syslog.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <array>
#include <limits>
#include <atomic>

#include "Daemon.h"

static std::atomic_bool stop_flag(false);

#define make_str(x) (((std::stringstream::__stringbuf_type*)(std::stringstream() << x).rdbuf())->str())

size_t count_files(const std::string& path)
{
    DIR *dp;
    size_t counter = 0;
    struct dirent *ep;     
    dp = opendir(path.c_str());

    if (dp != NULL)
    {
        while (ep = readdir(dp))
            counter++;
        closedir(dp);

        return counter;
    }

    return 0;
}

void read_memqueue_device(const std::string& path)
{
    const auto max_buffer_size = std::numeric_limits<unsigned short>::max();
    std::array<char, max_buffer_size> buffer;
    const auto prefix = path + "/memqueue_elem_";

    int fd = open("/dev/memqueue", O_RDONLY);
    if (fd == -1)
        throw std::runtime_error(make_str("/dev/memqueue open failed with error " << errno));

    auto counter = count_files(path);

    ::syslog(LOG_USER | LOG_INFO, "started");

    while (stop_flag == false)
    {
        ssize_t n_bytes = read(fd, buffer.data(), max_buffer_size);

        if (n_bytes > 0)
        {
            auto file_name = prefix + std::to_string(counter);
            // ::syslog(LOG_USER | LOG_DEBUG, "n_bytes = %lu, counter = %lu, file = %s", n_bytes, counter, file_name.c_str());

            std::ofstream tmp_file;
            tmp_file.open(file_name);
            tmp_file.write(buffer.data(), n_bytes);
            tmp_file.close();

            counter++;
        }
        else
        {
            usleep(1000);
        }
    }

    close(fd);

    ::syslog(LOG_USER | LOG_INFO, "done");
}

void print_usage(const char * appName)
{
    std::cout << "usage: " << appName << " <path to dir>" << std::endl;
}

int main(int argc, char** argv)
{
    try
    {
        if (argc < 2)
        {
            print_usage(argv[0]);
            return 1;
        }

        ::openlog("memqueue_daemon", LOG_PID, 0);

        Daemon::daemonize([&]()
        {
            stop_flag = true;
        });

        read_memqueue_device(argv[1]);
    }
    catch (std::exception & ex)
    {
        ::syslog(LOG_USER | LOG_ERR, "%s", ex.what());
        return 1;
    }

    return 0;
}
