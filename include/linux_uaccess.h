/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#pragma once

#ifdef __KERNEL__
    #include <linux/uaccess.h>
#else
    #include <string.h>
    #define copy_to_user(to, from, n) (memcpy(to, from, n), 0)
    #define copy_from_user(to, from, n) (memcpy(to, from, n), 0)
#endif
