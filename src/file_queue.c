/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
// #include <linux/fs.h>
// #include <linux/fcntl.h>

#include "../include/linux_base.h"
#include "../include/linux_spinlock.h"
#include "../include/linux_syscalls.h"

#include "../include/memqueue_constants.h"
#include "../include/file_queue.h"

#define HEADER_SIZE (sizeof(loff_t) + sizeof(loff_t))
#define HEADER_READ_OFFSET  0
#define HEADER_WRITE_OFFSET sizeof(loff_t)

//TODO: try this functions
// Since version 4.14 of Linux kernel, vfs_read and vfs_write 
// functions are no longer exported for use in modules. 
// Instead, functions exclusively for kernel's file access are provided:
// Also, filp_open no longer accepts user-space string, 
// so it can be used for kernel access directly (without dance with set_fs).
// # Read the file from the kernel space.
// ssize_t kernel_read(struct file *file, void *buf, size_t count, loff_t *pos);
// # Write the file from the kernel space.
// ssize_t kernel_write(struct file *file, const void *buf, size_t count,loff_t *pos);

// ========== internal variables ========== 
static file_descriptor queue = 0;
static size_t queue_size = 0;
static size_t file_size = 0;
static mm_segment_t oldfs;

static loff_t queue_pos_begin = 0;
static loff_t queue_pos_end   = 0;
static loff_t queue_pos_read  = 0;
static loff_t queue_pos_write = 0;

static DEFINE_SPINLOCK(lock_pos);
static DEFINE_SPINLOCK(lock_read);
static DEFINE_SPINLOCK(lock_write);

// ========== prototypes for internal functions ========== 

static ssize_t read_block(loff_t pos_read, char * data, size_t size);
static loff_t  read_data (loff_t pos_read, char * data, size_t length);
static loff_t  read_bytes(loff_t pos_read, char * data, size_t length);

static ssize_t write_block(loff_t pos_write, const char * data, size_t length);
static loff_t  write_data (loff_t pos_write, const char * data, size_t length);
static loff_t  write_bytes(loff_t pos_write,       char * data, size_t length);

static bool check_empty_space (loff_t pos_read, loff_t pos_write, size_t length);
static bool check_filled_space(loff_t pos_read, loff_t pos_write);

// ========== base functions ==========

int filequeue_open(const char * path, size_t _queue_size)
{
    int ret_code = 0;

    queue_size      = _queue_size;
    file_size       = HEADER_SIZE + queue_size;
    queue_pos_begin = HEADER_SIZE;
    queue_pos_end   = HEADER_SIZE + queue_size;
    queue_pos_read  = HEADER_SIZE;
    queue_pos_write = HEADER_SIZE;    

    oldfs = get_fs();
    set_fs(get_ds());
    queue = filp_open(path, O_RDWR, 0600);
    set_fs(oldfs);

    if (IS_ERR(queue))
    {
        oldfs = get_fs();
        set_fs(get_ds());
        queue = filp_open(path, O_RDWR | O_CREAT, 0600);
        set_fs(oldfs);

        if (IS_ERR(queue))
            return -EIO;

        ret_code = vfs_fallocate(queue, 0, 0, file_size);
        if (ret_code != 0)
            return ret_code;
    }
    else
    {
        ret_code = read_bytes(HEADER_READ_OFFSET, (char*)&queue_pos_read, sizeof(loff_t));
        if (ret_code < 0)
            return ret_code;

        ret_code = read_bytes(HEADER_WRITE_OFFSET, (char*)&queue_pos_write, sizeof(loff_t));
        if (ret_code < 0)
            return ret_code;
    }

    {
        loff_t size = vfs_llseek(queue, 0L, SEEK_END);
        if (size != file_size)
            return -EINVAL;
    }

    INIT_SPINLOCK(lock_pos);
    INIT_SPINLOCK(lock_read);
    INIT_SPINLOCK(lock_write);

    return 0;
}

int filequeue_close(void)
{
    int ret_code = 0;

    if (IS_ERR(queue) == false)
    {
        ret_code = write_bytes(HEADER_READ_OFFSET, (char*)&queue_pos_read, sizeof(loff_t));
        if (ret_code < 0)
            return ret_code;

        ret_code = write_bytes(HEADER_WRITE_OFFSET, (char*)&queue_pos_write, sizeof(loff_t));
        if (ret_code < 0)
            return ret_code;

        oldfs = get_fs();
        set_fs(get_ds());
        filp_close(queue, NULL);
        set_fs(oldfs);
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

    return 0;
}

// ========== read functions ==========

ssize_t filequeue_read(char * data, size_t size)
{
    ssize_t ret_code = 0;
    loff_t pos_read  = 0;
    loff_t pos_write = 0;

    if (data == 0 || size == 0)
        return -EINVAL;

    spin_lock(&lock_read);

    spin_lock(&lock_pos);
    pos_read  = queue_pos_read;
    pos_write = queue_pos_write;
    spin_unlock(&lock_pos);

    PRINTF(KERN_DEBUG, "filequeue positions before read %lli %lli\n", 
        pos_read - queue_pos_begin, 
        pos_write - queue_pos_begin
    );

    if (check_filled_space(pos_read, pos_write))
    {
        ret_code = read_block(pos_read, data, size);
    }

    spin_unlock(&lock_read);
    return ret_code;
}

static ssize_t read_block(loff_t pos_read, char * data, size_t size)
{
    size_t length = 0;

    pos_read = read_data(pos_read, (char*)&length, sizeof(size_t));
    if (pos_read < 0)
        return pos_read;
    if (length > size)
        return -ENOSPC;

    pos_read = read_data(pos_read, data, length);
    if (pos_read < 0)
        return pos_read;

    spin_lock(&lock_pos);
    queue_pos_read = pos_read;
    spin_unlock(&lock_pos);

    return length;
}

static loff_t read_data(loff_t pos_read, char * data, size_t length)
{
    size_t length_tail = queue_pos_end - pos_read;

    if (length_tail < length)
    {
        size_t length_head = length - length_tail;

        pos_read = read_bytes(pos_read, data, length_tail);
        if (pos_read < 0)
            return pos_read;

        return read_bytes(queue_pos_begin, data + length_tail, length_head);
    }
    else
    {
        return read_bytes(pos_read, data, length);
    }
}

static loff_t read_bytes(loff_t pos_read, char * data, size_t length)
{
    ssize_t n_bytes = 0;
    size_t offset = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    while (length)
    {
        n_bytes = vfs_read(queue, data + offset, length, &pos_read);
        if (n_bytes < 0)
            break;
        length   -= n_bytes;
        offset   += n_bytes;
        pos_read += n_bytes;
    }
    set_fs(oldfs);

    if (n_bytes < 0)
        return n_bytes;
    return pos_read == queue_pos_end ? queue_pos_begin : pos_read;
}

// ========== write functions ==========

ssize_t filequeue_write(const char * data, size_t length)
{
    ssize_t ret_code = 0;
    loff_t pos_read  = 0;
    loff_t pos_write = 0;

    if (data == 0 || length == 0)
        return -EINVAL;

    spin_lock(&lock_write);

    spin_lock(&lock_pos);
    pos_read  = queue_pos_read;
    pos_write = queue_pos_write;
    spin_unlock(&lock_pos);

    PRINTF(KERN_DEBUG, "filequeue positions before write %lli %lli\n", 
        pos_read - queue_pos_begin, 
        pos_write - queue_pos_begin
    );
    
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

static ssize_t write_block(loff_t pos_write, const char * data, size_t length)
{
    pos_write = write_data(pos_write, (char*)&length, sizeof(size_t));
    if (pos_write < 0)
        return pos_write;

    pos_write = write_data(pos_write, data, length);
    if (pos_write < 0)
        return pos_write;

    spin_lock(&lock_pos);
    queue_pos_write = pos_write;
    spin_unlock(&lock_pos);

    return length;
}

static loff_t write_data(loff_t pos_write, const char * data, size_t length)
{
    size_t length_tail = queue_pos_end - pos_write;

    if (length_tail < length)
    {
        size_t length_head = length - length_tail;

        pos_write = write_bytes(pos_write, (char*)data, length_tail);
        if (pos_write < 0)
            return pos_write;
        
        return write_bytes(queue_pos_begin, (char*)(data + length_tail), length_head);
    }
    else
    {
        return write_bytes(pos_write, (char*)data, length);
    }
}

static loff_t write_bytes(loff_t pos_write, char * data, size_t length)
{
    ssize_t n_bytes = 0;
    size_t offset = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    while (length)
    {
        n_bytes = vfs_write(queue, data + offset, length, &pos_write);
        if (n_bytes < 0)
            break;
        length    -= n_bytes;
        offset    += n_bytes;
        pos_write += n_bytes;
    }
    set_fs(oldfs);

    if (n_bytes < 0)
        return n_bytes;
    return pos_write == queue_pos_end ? queue_pos_begin : pos_write;
}

// ========== check functions ==========

static bool check_empty_space(loff_t pos_read, loff_t pos_write, size_t length)
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

static bool check_filled_space(loff_t pos_read, loff_t pos_write)
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
