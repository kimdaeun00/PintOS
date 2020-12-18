#ifndef _FRAME_HEADER_H
#define _FRAME_HEADER_H

#include <debug.h>
#include <stdint.h>
#include "lib/kernel/hash.h"
#include "vm/page.h"
#include "threads/palloc.h"

struct lock ft_lock;

struct fte{
    void * kpage;
    struct spte* spte;
    struct thread* t;
    struct list_elem elem;
    bool inevictable;    //install_new_fte에서 false로 초기화
};

void ft_init(void);
void fte_destroy(struct fte*);
struct fte* spte_to_fte(struct spte*);
void * get_kpage(enum palloc_flags);
struct fte* frame_alloc(struct spte*, enum palloc_flags);
struct fte* frame_alloc_exec(struct spte*, enum palloc_flags,struct fte*);
struct fte* frame_alloc_swap(struct spte* , enum palloc_flags,struct fte*);
void * find_evict(void);
void spt_exit(struct hash *);
struct fte* install_new_fte(void *, struct spte*);
// void set_evict_file(void *, unsigned , bool);
#endif