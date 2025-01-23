#pragma once

#include <stddef.h>

void *heap_alloc(size_t size);
void *heap_realloc(void *ptr, size_t size);
void heap_free(void *address);