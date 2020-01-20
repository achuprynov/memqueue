/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>

#include "memqueue_constants.h"
#include "file_storage.h"

#define HEADER_SIZE (sizeof(size_t) * 3)
#define HEADER_READ_FILE_OFFSET  0
#define HEADER_READ_USER_OFFSET  sizeof(size_t)
#define HEADER_WRITE_USER_OFFSET (sizeof(size_t) + sizeof(size_t))


// ========== internal variables ==========

static struct file *storage = NULL;
static size_t file_size;
static mm_segment_t oldfs;

// ========== prototypes for internal functions ========== 

int write_read_file_offset  (size_t val);
int write_read_user_offset  (size_t val);
int write_write_user_offset (size_t val);
int write_header_value(size_t val, size_t offset);

// ========== base functions ==========

int filestorage_open(const char * path, size_t size)
{
    int ret_code = 0;
    file_size = size;

    oldfs = get_fs();
    set_fs(get_ds());
    storage = filp_open(path, O_RDWR, 0);
    set_fs(oldfs);

    if (IS_ERR(storage))
    {
        oldfs = get_fs();
        set_fs(get_ds());
        storage = filp_open(path, O_RDWR | O_CREAT, 0000);
        set_fs(oldfs);

        if (IS_ERR(storage))
            return EIO;

        ret_code = vfs_fallocate(storage, 0, 0, file_size);
        if (ret_code != 0)
            return ret_code;

        return filestorage_write_offsets(HEADER_SIZE, HEADER_SIZE, HEADER_SIZE);
    }

    return 0;
}

void filestorage_close(void)
{
    if (IS_ERR(storage) == false)
    {
        oldfs = get_fs();
        set_fs(get_ds());
        filp_close(storage, NULL);
        set_fs(oldfs);
    }
}

// ========== read/write functions ==========

int filestorage_read(void * data, size_t size)
{
    ssize_t n_bytes = 0;
    size_t offset = 0;
    loff_t pos = 0;

    if (size != file_size)
        return EINVAL;

    oldfs = get_fs();
    set_fs(get_ds());
    while (size)
    {
        n_bytes = vfs_read(storage, data + offset, size, &pos);
        if (n_bytes < 0)
            break;
        size   -= n_bytes;
        offset += n_bytes;
    }
    set_fs(oldfs);

    return n_bytes < 0 ? n_bytes : 0;
}

int filestorage_write_length_value(unsigned short val, size_t offset)
{
    ssize_t n_bytes = 0;
    loff_t pos = offset;

    oldfs = get_fs();
    set_fs(get_ds());
    n_bytes = vfs_write(storage, (void *)&val, sizeof(val), &pos);
    set_fs(oldfs);

    return n_bytes == sizeof(val) ? 0 : n_bytes;
}

int filestorage_write(const void *data, size_t offset, unsigned short length)
{
    ssize_t n_bytes = 0;
    loff_t pos = offset;

    if (length + offset > file_size)
        return -EINVAL;

    offset = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    while (length)
    {
        n_bytes = vfs_write(storage, data + offset, length, &pos);
        if (n_bytes < 0)
            break;
        length -= n_bytes;
        offset += n_bytes;
        pos    += n_bytes;
    }
    set_fs(oldfs);

    return n_bytes < 0 ? n_bytes : offset;
}

int filestorage_write_offsets(size_t offset_read_file, size_t offset_read_user, size_t offset_write_user)
{
    int ret_code = write_read_file_offset(offset_read_file);
    if (ret_code != 0)
        return ret_code;

    ret_code = write_read_user_offset(offset_read_user);
    if (ret_code != 0)
        return ret_code;

    return write_write_user_offset(offset_write_user);
}

// ========== set functions ==========

int write_read_file_offset(size_t val)
{
    return write_header_value(val, HEADER_READ_FILE_OFFSET);
}
int write_read_user_offset (size_t val)
{
    return write_header_value(val, HEADER_READ_USER_OFFSET);
}
int write_write_user_offset(size_t val)
{
    return write_header_value(val, HEADER_WRITE_USER_OFFSET);
}
int write_header_value(size_t val, size_t offset)
{
    ssize_t n_bytes = 0;
    loff_t pos = offset;

    oldfs = get_fs();
    set_fs(get_ds());
    n_bytes = vfs_write(storage, (void *)&val, sizeof(val), &pos);
    set_fs(oldfs);

    return n_bytes == sizeof(val) ? 0 : n_bytes;
}