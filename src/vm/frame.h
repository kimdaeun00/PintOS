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
    struct hash_elem elem;
};

unsigned ft_hash_func(const struct hash_elem *, void *);
bool ft_less(const struct hash_elem *, const struct hash_elem *, void *);
void ft_init(void);
struct fte* frame_alloc(struct spte*, enum palloc_flags);
struct fte* frame_alloc_exec(struct spte*, enum palloc_flags);
struct fte* frame_alloc_swap(struct spte* , enum palloc_flags);
#endif