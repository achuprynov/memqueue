/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#include "../include/linux_base.h"
#include "../include/linux_mm.h"
#include "../include/linux_uaccess.h"
#include "../include/linux_spinlock.h"

#include "../include/memqueue_constants.h"
#include "../include/mem_queue.h"

// ========== internal variables ==========

static size_t queue_size = 0;
static char * queue = 0;

static char * queue_pos_begin = 0;
static char * queue_pos_end   = 0;
static char * queue_pos_read  = 0;
static char * queue_pos_write = 0;

static DEFINE_SPINLOCK(lock_pos);
static DEFINE_SPINLOCK(lock_read);
static DEFINE_SPINLOCK(lock_write);

// ========== prototypes for internal functions ========== 

static ssize_t  read_block(char * pos_read,        char * data, size_t size);
static ssize_t write_block(char * pos_write, const char * data, size_t length);

static char * copy_kern_bytes(char * data, char * pos_read, char * pos_write, size_t length);
static char * copy_user_bytes(char * data, char * pos_read, char * pos_write, size_t length);

static bool check_empty_space (char * pos_read, char * pos_write, size_t length);
static bool check_filled_space(char * pos_read, char * pos_write);

// ========== base functions ==========

int memqueue_open(size_t _queue_size)
{
    queue = kvmalloc(_queue_size, GFP_KERNEL);
    if (queue == 0)
        return ENOMEM;

    queue_size      = _queue_size;
    queue_pos_begin = queue;
    queue_pos_end   = queue_pos_begin + queue_size;
    queue_pos_read  = queue_pos_begin;
    queue_pos_write = queue_pos_begin;    

    INIT_SPINLOCK(lock_pos);
    INIT_SPINLOCK(lock_read);
    INIT_SPINLOCK(lock_write);

    return 0;
}

void memqueue_close(void)
{
    if (queue)
    {
        kvfree(queue);
        queue = 0;
    }

    queue_size      = 0;
    queue_pos_begin = 0;
    queue_pos_end   = 0;
    queue_pos_read  = 0;
    queue_pos_write = 0;

    DESTROY_SPINLOCK(lock_pos);
    DESTROY_SPINLOCK(lock_read);
    DESTROY_SPINLOCK(lock_write);
}

// ========== read functions ==========

ssize_t memqueue_read(char * data, size_t size)
{
    ssize_t ret_code = 0;
    char * pos_read  = 0;
    char * pos_write = 0;

    if (data == 0 || size == 0)
        return -EINVAL;

    spin_lock(&lock_read);

    spin_lock(&lock_pos);
    pos_read  = queue_pos_read;
    pos_write = queue_pos_write;
    spin_unlock(&lock_pos);

    // PRINTF(KERN_DEBUG, "memqueue positions before read %lu %lu\n", 
    //     pos_read  - queue_pos_begin, 
    //     pos_write - queue_pos_begin
    // );

    if (check_filled_space(pos_read, pos_write))
    {
        ret_code = read_block(pos_read, data, size);
    }

    spin_unlock(&lock_read);
    return ret_code;
}

static ssize_t read_block(char * pos_read, char * data, size_t size)
{
    size_t length = 0;

    pos_read = copy_kern_bytes((char*)&length, pos_read, 0, sizeof(size_t));
    if (length > size)
        return -ENOSPC;

    pos_read = copy_user_bytes(data, pos_read, 0, length);

    if (pos_read)
    {
        spin_lock(&lock_pos);
        queue_pos_read = pos_read;
        spin_unlock(&lock_pos);
        return length;
    }

    return 0;
}

// ========== write functions ==========

ssize_t memqueue_write(const char * data, size_t length)
{
    ssize_t ret_code = 0;
    char * pos_read  = 0;
    char * pos_write = 0;

    if (data == 0 || length == 0)
        return -EINVAL;

    spin_lock(&lock_write);

    spin_lock(&lock_pos);
    pos_read  = queue_pos_read;
    pos_write = queue_pos_write;
    spin_unlock(&lock_pos);

    // PRINTF(KERN_DEBUG, "memqueue positions before write %lu %lu\n", 
    //     pos_read  - queue_pos_begin, 
    //     pos_write - queue_pos_begin
    // );

    if (check_empty_space(pos_read, pos_write, length))
    {
        ret_code = write_block(pos_write, data, length);
    }
    else
    {
        ret_code = -ENOSPC;
    }

    spin_unlock(&lock_write);
    return ret_code;
}

static ssize_t write_block(char * pos_write, const char * data, size_t length)
{
    pos_write = copy_kern_bytes((char*)&length, 0, pos_write, sizeof(size_t));

    pos_write = copy_user_bytes((char*)data, 0, pos_write, length);

    if (pos_write)
    {
        spin_lock(&lock_pos);
        queue_pos_write = pos_write;
        spin_unlock(&lock_pos);
        return length;
    }

    return 0;
}

// ========== copy bytes functions ==========

static char * copy_kern_bytes(char * data, char * pos_read, char * pos_write, size_t length)
{
    size_t length_tail = queue_pos_end - (pos_read > 0 ? pos_read : pos_write);

    if (pos_read != 0 && pos_write != 0)
        return 0;
    if (pos_read == 0 && pos_write == 0)
        return 0;

    if (length_tail < length)
    {
        size_t length_head = length - length_tail;
        if (pos_read) {
            memcpy(data,               pos_read,        length_tail);
            memcpy(data + length_tail, queue_pos_begin, length_head);
        } else {
            memcpy(pos_write,       data,               length_tail);
            memcpy(queue_pos_begin, data + length_tail, length_head);
        }
        return queue_pos_begin + length_head;
    }
    else
    {
        char * tmp_pos = 0;
        if (pos_read) {
            memcpy(data, pos_read, length);
            tmp_pos = pos_read + length;
        } else {
            memcpy(pos_write, data, length);
            tmp_pos = pos_write + length;
        }
        return tmp_pos == queue_pos_end ? queue_pos_begin : tmp_pos;
    }
}

static char * copy_user_bytes(char * data, char * pos_read, char * pos_write, size_t length)
{
    size_t length_tail = queue_pos_end - (pos_read > 0 ? pos_read : pos_write);

    if (pos_read != 0 && pos_write != 0)
        return 0;
    if (pos_read == 0 && pos_write == 0)
        return 0;

    if (length_tail < length)
    {
        size_t length_head = length - length_tail;
        if (pos_read) {
            if (copy_to_user(data,               pos_read,        length_tail) != 0) return 0;
            if (copy_to_user(data + length_tail, queue_pos_begin, length_head) != 0) return 0;
        } else {
            if (copy_from_user(pos_write,       data,               length_tail) != 0) return 0;
            if (copy_from_user(queue_pos_begin, data + length_tail, length_head) != 0) return 0;
        }
        return queue_pos_begin + length_head;
    }
    else
    {
        char * tmp_pos = 0;
        if (pos_read) {
            if (copy_to_user(data, pos_read, length) != 0) return 0;
            tmp_pos = pos_read + length;
        } else {
            if (copy_from_user(pos_write, data, length) != 0) return 0;
            tmp_pos = pos_write + length;
        }
        return tmp_pos == queue_pos_end ? queue_pos_begin : tmp_pos;
    }
}

// ========== check functions ==========

static bool check_empty_space(char * pos_read, char * pos_write, size_t length)
{
    size_t empty_space = 0;

    if (pos_read == pos_write)
    {
        empty_space = queue_size;
    }
    else if (pos_read < pos_write)
    {// --------------================----------------X
     //               ^pos_read       ^pos_write      ^pos_end
        empty_space = (pos_read - queue_pos_begin) + (queue_pos_end - pos_write);
    }
    else if (pos_read > pos_write)
    {// ==============----------------================X
     //               ^pos_write      ^pos_read       ^pos_end
        empty_space = pos_read - pos_write;
    }

    // pos_write always be less pos_read if writing more frequently then reading
    // otherwise condition "if (pos_read == pos_write)" (see above) will be wrong
    return empty_space > (sizeof(size_t) + length);
}

static bool check_filled_space(char * pos_read, char * pos_write)
{
    if (pos_read == pos_write)
    {
        return false;
    }
    else
    {// --------------================----------------X
     //               ^pos_read       ^pos_write      ^pos_end
     // or
     // ==============----------------================X
     //               ^pos_write      ^pos_read       ^pos_end
        return true;
    }
}
