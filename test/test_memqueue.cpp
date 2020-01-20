/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#define BOOST_TEST_MODULE MemQueueTestModule
#include <boost/test/included/unit_test.hpp>
#include <string>
#include <list>
#include <stack>

#include "../include/mem_queue.h"

BOOST_AUTO_TEST_SUITE(MemQueueTest)

BOOST_AUTO_TEST_CASE(MemQueueBaseTest)
{
    const size_t queue_size  = 1000;
    const size_t buffer_size = 100;
    std::array<char, buffer_size> r_buffer;
    std::array<char, buffer_size> w_buffer;

    auto result = memqueue_open(queue_size);
    BOOST_CHECK_EQUAL(result, 0);

    auto n_bytes = memqueue_read(r_buffer.data(), buffer_size);
    BOOST_CHECK_EQUAL(n_bytes, 0);

    w_buffer.fill('a');
    n_bytes = memqueue_write(w_buffer.data(), buffer_size);
    BOOST_CHECK_EQUAL(n_bytes, buffer_size);

    r_buffer.fill(0);
    n_bytes = memqueue_read(r_buffer.data(), buffer_size);
    BOOST_CHECK_EQUAL(n_bytes, buffer_size);

    BOOST_TEST(r_buffer == w_buffer);

    memqueue_close();
}

BOOST_AUTO_TEST_CASE(MemQueueWriteReadTest)
{
    ssize_t n_bytes = 0;
    auto n_times = 1000;
    const size_t queue_size  = 1000;
    const size_t buffer_size = 100;
    std::array<char, buffer_size> r_buffer;
    std::array<char, buffer_size> w_buffer;

    auto result = memqueue_open(queue_size);
    BOOST_CHECK_EQUAL(result, 0);

    w_buffer.fill('a');

    while (n_times--)
    {
        n_bytes = memqueue_write(w_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);

        r_buffer.fill(0);
        n_bytes = memqueue_read(r_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);

        BOOST_TEST(r_buffer == w_buffer);
    }

    memqueue_close();
}

BOOST_AUTO_TEST_CASE(MemQueueWriteToFullThenReadTest)
{
    ssize_t n_bytes = 0;
    auto n_times = 1000;
    const size_t queue_size  = 1000;
    const size_t buffer_size = 100;
    std::array<char, buffer_size> r_buffer;
    std::array<char, buffer_size> w_buffer;

    auto result = memqueue_open(queue_size);
    BOOST_CHECK_EQUAL(result, 0);

    w_buffer.fill('a');

    while (n_times--)
    {
        auto counter = 0;
        while (true) {
            n_bytes = memqueue_write(w_buffer.data(), buffer_size);
            if (n_bytes != buffer_size) {
                BOOST_CHECK_EQUAL(n_bytes, -ENOSPC);
                break;
            }
            counter++;
        }

        size_t n = queue_size / (buffer_size + sizeof(size_t));
        BOOST_CHECK_EQUAL(counter, n);

        while (true) {
            r_buffer.fill(0);
            n_bytes = memqueue_read(r_buffer.data(), buffer_size);
            if (n_bytes != buffer_size) {
                BOOST_CHECK_EQUAL(n_bytes, 0);
                break;
            }
            BOOST_TEST(r_buffer == w_buffer);
            counter--;
        }

        BOOST_CHECK_EQUAL(counter, 0);
    }

    memqueue_close();
}

BOOST_AUTO_TEST_CASE(MemQueueWriteReadMaxTest)
{
    ssize_t n_bytes = 0;
    auto n_times = 1000;
    const size_t queue_size  = 1000;
    const size_t buffer_size = queue_size - (sizeof(size_t) + 1);
    std::array<char, buffer_size> r_buffer;
    std::array<char, buffer_size> w_buffer;

    auto result = memqueue_open(queue_size);
    BOOST_CHECK_EQUAL(result, 0);

    w_buffer.fill('a');

    while (n_times--)
    {
        n_bytes = memqueue_write(w_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);

        r_buffer.fill(0);
        n_bytes = memqueue_read(r_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, buffer_size);

        BOOST_TEST(r_buffer == w_buffer);
    }

    memqueue_close();
}

BOOST_AUTO_TEST_CASE(MemQueueWriteReadMaxPlusTest)
{
    ssize_t n_bytes = 0;
    auto n_times = 1000;
    const size_t queue_size  = 1000;
    const size_t buffer_size = queue_size + sizeof(size_t);
    std::array<char, buffer_size> r_buffer;
    std::array<char, buffer_size> w_buffer;

    auto result = memqueue_open(queue_size);
    BOOST_CHECK_EQUAL(result, 0);

    w_buffer.fill('a');

    while (n_times--)
    {
        n_bytes = memqueue_write(w_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, -ENOSPC);

        r_buffer.fill(0);
        n_bytes = memqueue_read(r_buffer.data(), buffer_size);
        BOOST_CHECK_EQUAL(n_bytes, 0);
    }

    memqueue_close();
}

BOOST_AUTO_TEST_SUITE_END()
