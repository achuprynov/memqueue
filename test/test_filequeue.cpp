/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#define BOOST_TEST_MODULE FileQueueTestModule
#include <boost/test/included/unit_test.hpp>
#include <string>
#include <list>
#include <stack>
#include <stdio.h>

#include "../include/file_queue.h"

static const char * path = "/var/tmp/filequeue";

BOOST_AUTO_TEST_SUITE(FileQueueTest)

BOOST_AUTO_TEST_CASE(FileQueueBaseTest)
{
    const size_t queue_size  = 1000;
    const size_t buffer_size = 100;
    std::array<char, buffer_size> r_buffer;
    std::array<char, buffer_size> w_buffer;

    remove(path);
    {
        auto result = filequeue_open(path, queue_size);
        BOOST_CHECK_EQUAL(result, 0);

        auto n_bytes = filequeue_read(r_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, 0);

        w_buffer.fill('a');
        n_bytes = filequeue_write(w_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);

        r_buffer.fill(0);
        n_bytes = filequeue_read(r_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);
        BOOST_TEST(r_buffer == w_buffer);

        n_bytes = filequeue_write(w_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);

        filequeue_close();
    }

    {
        auto result = filequeue_open(path, queue_size);
        BOOST_CHECK_EQUAL(result, 0);

        r_buffer.fill(0);
        auto n_bytes = filequeue_read(r_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);
        BOOST_TEST(r_buffer == w_buffer);

        filequeue_close();
    }

    {
        auto result = filequeue_open(path, queue_size + 1);
        BOOST_CHECK_EQUAL(result, -EINVAL);
        filequeue_close();
    }
    remove(path);
}

BOOST_AUTO_TEST_CASE(FileQueueWriteReadTest)
{
    ssize_t n_bytes = 0;
    auto n_times = 1000;
    const size_t queue_size  = 1000;
    const size_t buffer_size = 100;
    std::array<char, buffer_size> r_buffer;
    std::array<char, buffer_size> w_buffer;

    remove(path);
    auto result = filequeue_open(path, queue_size);
    BOOST_CHECK_EQUAL(result, 0);

    w_buffer.fill('a');

    while (n_times--)
    {
        n_bytes = filequeue_write(w_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);

        r_buffer.fill(0);
        n_bytes = filequeue_read(r_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);

        BOOST_TEST(r_buffer == w_buffer);
    }

    filequeue_close();
    remove(path);
}

BOOST_AUTO_TEST_CASE(FileQueueWriteToFullThenReadTest)
{
    ssize_t n_bytes = 0;
    auto n_times = 1000;
    const size_t queue_size  = 1000;
    const size_t buffer_size = 100;
    std::array<char, buffer_size> r_buffer;
    std::array<char, buffer_size> w_buffer;

    remove(path);
    auto result = filequeue_open(path, queue_size);
    BOOST_CHECK_EQUAL(result, 0);

    w_buffer.fill('a');

    while (n_times--)
    {
        auto counter = 0;
        while (true) {
            n_bytes = filequeue_write(w_buffer.data(), buffer_size);
            if (n_bytes != buffer_size) {
                BOOST_CHECK_EQUAL(n_bytes, -ENOSPC);
                break;
            }
            counter++;
        }

        size_t n = queue_size / (buffer_size + sizeof(size_t));
        BOOST_CHECK_EQUAL(counter, n);

        if (n_times % 10 == 0)
        {
            filequeue_close();
            result = filequeue_open(path, queue_size);
            BOOST_CHECK_EQUAL(result, 0);
        }

        while (true) {
            r_buffer.fill(0);
            n_bytes = filequeue_read(r_buffer.data(), buffer_size);
            if (n_bytes != buffer_size) {
                BOOST_CHECK_EQUAL(n_bytes, 0);
                break;
            }
            BOOST_TEST(r_buffer == w_buffer);
            counter--;
        }

        BOOST_CHECK_EQUAL(counter, 0);
    }

    filequeue_close();
    remove(path);
}

BOOST_AUTO_TEST_CASE(FileQueueWriteReadMaxTest)
{
    ssize_t n_bytes = 0;
    auto n_times = 1000;
    const size_t queue_size  = 1000;
    const size_t buffer_size = queue_size - (sizeof(size_t) + 1);
    std::array<char, buffer_size> r_buffer;
    std::array<char, buffer_size> w_buffer;

    remove(path);
    auto result = filequeue_open(path, queue_size);
    BOOST_CHECK_EQUAL(result, 0);

    w_buffer.fill('a');

    while (n_times--)
    {
        n_bytes = filequeue_write(w_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);

        r_buffer.fill(0);
        n_bytes = filequeue_read(r_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);

        BOOST_TEST(r_buffer == w_buffer);
    }

    filequeue_close();
    remove(path);
}

BOOST_AUTO_TEST_CASE(FileQueueWriteReadMaxPlusTest)
{
    ssize_t n_bytes = 0;
    auto n_times = 1000;
    const size_t queue_size  = 1000;
    const size_t buffer_size = queue_size + sizeof(size_t);
    std::array<char, buffer_size> r_buffer;
    std::array<char, buffer_size> w_buffer;

    remove(path);
    auto result = filequeue_open(path, queue_size);
    BOOST_CHECK_EQUAL(result, 0);

    w_buffer.fill('a');

    while (n_times--)
    {
        n_bytes = filequeue_write(w_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, -ENOSPC);

        r_buffer.fill(0);
        n_bytes = filequeue_read(r_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, 0);
    }

    filequeue_close();
    remove(path);
}

BOOST_AUTO_TEST_SUITE_END()
