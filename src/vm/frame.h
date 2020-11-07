#ifndef _FRAME_HEADER_H
#define _FRAME_HEADER_H

#include <debug.h>
#include <stdint.h>
#include "lib/kernel/hash.h"
#include "vm/page.h"
#include "threads/palloc.h"

struct fte{
    void * kpage;
    struct spte* spte;
    struct thread* t;
    struct list_elem elem;
    struct list_elem evict_elem;
};

void evict_init(void);
void ft_init(void);
struct fte* frame_alloc(struct spte*, enum palloc_flags);
struct fte* frame_alloc_exec(struct spte*, enum palloc_flags,struct fte*);
struct fte* frame_alloc_swap(struct spte* , enum palloc_flags,struct fte*);
void * find_evict(void);
void spt_exit(struct thread*);
#endif