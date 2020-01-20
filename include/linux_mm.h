/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#pragma once

#ifdef __KERNEL__
    #include <linux/mm.h>
#else
    #include <stdlib.h>
    #define kvmalloc(size, flags) malloc(size)
    #define kvfree(ptr) free(ptr)
#endif
