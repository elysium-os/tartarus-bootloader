#include "heap.h"

#include "common/panic.h"
#include "lib/math.h"
#include "lib/mem.h"
#include "memory/pmm.h"

#include <stdint.h>

#define MINIMUM_ENTRY_SIZE 8
#define EXTRA_ALLOC_PAGES 2

typedef struct heap_entry {
    size_t size;
    struct heap_entry *next, *last;
} heap_entry_t;

heap_entry_t *g_heap = NULL;

static void delete(heap_entry_t *entry) {
    if(g_heap == entry) g_heap = entry->next;
    if(entry->last != NULL) entry->last->next = entry->next;
    if(entry->next != NULL) entry->next->last = entry->last;
    entry->next = NULL;
    entry->last = NULL;
}

static void insert(heap_entry_t *entry) {
    entry->last = NULL;
    entry->next = g_heap;
    if(g_heap != NULL) g_heap->last = entry;
    g_heap = entry;
}

static void alloc(int pages) {
    heap_entry_t *new_entry = pmm_alloc(PMM_AREA_STANDARD, pages);
    new_entry->size = pages * PMM_GRANULARITY - sizeof(heap_entry_t);
    insert(new_entry);
}

void *heap_alloc(size_t size) {
    for(heap_entry_t *current_entry = g_heap; current_entry != NULL; current_entry = current_entry->next) {
        if(current_entry->size < size) continue;

        size_t left = current_entry->size - size;
        if(left >= sizeof(heap_entry_t) + MINIMUM_ENTRY_SIZE) {
            current_entry->size = size;

            heap_entry_t *new_entry = (heap_entry_t *) ((uintptr_t) current_entry + sizeof(heap_entry_t) + current_entry->size);
            new_entry->size = left - sizeof(heap_entry_t);
            insert(new_entry);
        }

        delete(current_entry);
        return (void *) ((uintptr_t) current_entry + sizeof(heap_entry_t));
    }
    alloc(MATH_DIV_CEIL(size + sizeof(heap_entry_t), PMM_GRANULARITY) + EXTRA_ALLOC_PAGES);
    return heap_alloc(size);
}

void *heap_realloc(void *ptr, size_t new_size) {
    void *new_ptr = heap_alloc(new_size);
    if(ptr == NULL) return new_ptr;
    heap_entry_t *entry = (heap_entry_t *) ((uintptr_t) ptr - sizeof(heap_entry_t));
    memcpy(new_ptr, ptr, new_size > entry->size ? entry->size : new_size);
    heap_free(ptr);
    return new_ptr;
}

void heap_free(void *address) {
    heap_entry_t *entry = (heap_entry_t *) ((uintptr_t) address - sizeof(heap_entry_t));
    for(heap_entry_t *current_entry = g_heap; current_entry != NULL; current_entry = current_entry->next) {
        if((uintptr_t) current_entry + sizeof(heap_entry_t) + current_entry->size == (uintptr_t) entry) {
            delete(current_entry);
            current_entry->size += entry->size + sizeof(heap_entry_t);
            entry = current_entry;
            continue;
        }

        if((uintptr_t) entry + sizeof(heap_entry_t) + entry->size == (uintptr_t) current_entry) {
            delete(current_entry);
            entry->size += current_entry->size + sizeof(heap_entry_t);
        }
    }
    insert(entry);
}
