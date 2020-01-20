/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#pragma once

#ifdef __KERNEL__
    #include <linux/syscalls.h>
    #define file_descriptor struct file *
#else
    #include <fcntl.h>
    #include <unistd.h>
    #define file_descriptor int
    #define mm_segment_t void*
    #define loff_t long long int
    #define get_fs(void) 0
    #define get_ds(void) 0
    #define set_fs(x) 
    #define filp_open(filename, flags, mode) open(filename, flags, mode)
    #define filp_close(fd, NULL) close(fd)
    #define IS_ERR(fd) (fd == -1)
    #define vfs_fallocate(fd, mode, offset, size) posix_fallocate(fd, offset, size)
    #define vfs_read(fd, buf, count, offset) pread(fd, buf, count, *offset)
    #define vfs_write(fd, buf, count, offset) pwrite(fd, buf, count, *offset)
    #define vfs_llseek(fd, offset, whence) lseek(fd, offset, whence)
#endif
