/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#include "../include/memqueue_constants.h"
#include "../include/mem_queue.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexander Chuprynov <achuprynov@gmail.com>");
MODULE_DESCRIPTION("Memory queue as Linux loadable kernel module (LKM).");
MODULE_VERSION("0.01");

// #define FILE_STORAGE_NAME "/var/tmp/memqueue"

// ========== prototypes for device functions ==========
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int major_num;

// This structure points to all of the device functions
static struct file_operations file_ops =
{
    .read    = device_read,
    .write   = device_write,
    .open    = device_open,
    .release = device_release
};

static ssize_t device_read(struct file *flip, char *dest, size_t len, loff_t *offset)
{
    return memqueue_read(dest, len);
}

static ssize_t device_write(struct file *flip, const char *src, size_t len, loff_t *offset)
{
    return memqueue_write(src, len);
}

static int device_open(struct inode *inode, struct file *file)
{
    try_module_get(THIS_MODULE);
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    module_put(THIS_MODULE);
    return 0;
}

static int __init memqueue_module_init(void)
{
    int ret_code = 0;
    size_t queue_size = 10240;

    major_num = register_chrdev(0, DEVICE_NAME, &file_ops);
    if (major_num < 0)
    {
        printk(KERN_ALERT "%s module could not register device: %d\n", DEVICE_NAME, major_num);
        return major_num;
    }

    printk(KERN_INFO "%s module registered with device major number %d\n", DEVICE_NAME, major_num);

    ret_code = memqueue_open(queue_size);
    if (ret_code == 0)
        printk(KERN_INFO "%s module opened. Queue size %lu.\n", DEVICE_NAME, queue_size);
    else
        printk(KERN_INFO "%s module failed with code %d\n", DEVICE_NAME, ret_code);

    return ret_code;
}

static void __exit memqueue_module_exit(void)
{
    memqueue_close();
    printk(KERN_INFO "%s module closed\n", DEVICE_NAME);

    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "%s unregistered\n", DEVICE_NAME);
}

module_init(memqueue_module_init);
module_exit(memqueue_module_exit);