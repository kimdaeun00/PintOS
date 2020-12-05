#ifndef _CACHE_HEADER_H
#define _CACHE_HEADER_H

#include <debug.h>
#include <stdint.h>
#include <devices/block.h>
#include <stdbool.h>
#include <list.h>
#include "threads/synch.h"

#define BUFFER_CACHE_SIZE 64
#define CACHE_SECTOR_SIZE 512

struct list cache_list;
struct cache_entry{
    bool dirty;
    bool accessed;
    bool is_start;
    char buffer[CACHE_SECTOR_SIZE];
    struct block *block;
    block_sector_t block_idx;
    struct list_elem elem;
};

void cache_init(void);
struct cache_entry *  is_hit(block_sector_t);
struct list_elem* clock_next(struct list* ,struct list_elem *);
void cache_write_back(struct cache_entry *);
void cache_evict(void);
struct cache_entry * set_cache(struct block *, block_sector_t);
void cache_write(struct block *, block_sector_t, const void *);
void cache_read(struct block *, block_sector_t, void *);
void cache_exit(void);

#endif