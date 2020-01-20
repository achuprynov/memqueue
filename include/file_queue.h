/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * On success, 0 is returned. 
 * On error, the number of error.
 */
int filequeue_open(const char * path, size_t _queue_size);

int filequeue_close(void);

/**
 * Read max <size> bytes from queue into a <data> array.
 * Return number of bytes read. 
 * If return value less than zero that indicates error. 
 * In this case abs(value) == number of error
 */
ssize_t filequeue_read(char * data, size_t size);

/**
  * Write <length> bytes into queue from a <data> array.
  * Return number of bytes written.
  * If return value less than zero that indicates error. 
  * In this case abs(value) == number of error
 */
ssize_t filequeue_write(const char * data, size_t length);

#ifdef __cplusplus
}
#endif
