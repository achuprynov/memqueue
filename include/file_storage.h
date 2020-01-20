/* 
 * Copyright (C) Alexander Chuprynov <achuprynov@gmail.com>
 * This file is part of solution of test task described in README.md.
 */
#pragma once

#include <stddef.h>

int filestorage_open(const char * path, size_t size);

void filestorage_close(void);

int filestorage_read (void * data, size_t size);

int filestorage_write_length_value(unsigned short val, size_t offset);

int filestorage_write(const void *data, size_t offset, unsigned short length);

int filestorage_write_offsets(size_t offset_read_file, size_t offset_read_user, size_t offset_write_user);
