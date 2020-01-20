/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#pragma once

#ifdef __KERNEL__
    #include <linux/stddef.h>
    #include <linux/errno.h>
    
    #define PRINTF(_level_, _fmt_, ...) printk(_level_ _fmt_, ##__VA_ARGS__)
#else
    #include <stdbool.h>
    #include <errno.h>
    #include <stdio.h>

    #define PRINTF(_level_, _fmt_, ...) printf(_fmt_, ##__VA_ARGS__)
#endif
