/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#pragma once

#ifdef __KERNEL__
    #include <linux/spinlock.h>
    #define INIT_SPINLOCK(spin) 
    #define DESTROY_SPINLOCK(spin) 
#else
    #include <pthread.h>
    #define DEFINE_SPINLOCK(spin) pthread_spinlock_t spin
    #define INIT_SPINLOCK(spin) pthread_spin_init(&spin, 0)
    #define DESTROY_SPINLOCK(spin) pthread_spin_destroy(&spin)

    #define spin_lock(spin) pthread_spin_lock(spin)
    #define spin_unlock(spin) pthread_spin_unlock(spin)
#endif