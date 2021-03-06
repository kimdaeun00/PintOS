#ifndef _PAGE_HEADER_H
#define _PAGE_HEADER_H

#include <debug.h>
#include <stdint.h>
#include "lib/kernel/hash.h"
#include "devices/block.h"


enum vm_status
 {
    VM_STK_GROW,
    VM_SWAP_DISK,
    VM_EXEC_FILE,
    VM_ON_MEMORY,
    VM_EXIT
 };

struct mmape{
    int mapid;
    void * addr;
    void * file_addr;
    struct file* file;
    struct spte* spte;
    struct list_elem elem;
};


struct spte{
    struct hash_elem elem;
    void * upage;
    struct file *file;
    uint32_t ofs;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
    int status;
    block_sector_t swap_index;
    bool dirty_bit;
};

void spt_hash_destroy(struct hash_elem *, void *);
unsigned spt_hash_func(const struct hash_elem *, void *);
bool spt_less(const struct hash_elem *, const struct hash_elem *, void *);
struct spte* spte_init(void * , enum vm_status,struct file *, uint32_t, uint32_t , uint32_t , bool );
struct spte* spt_get_spte(void *);
void mmape_init(int, void *, void *, struct file * , struct spte* );
void mmape_exit(struct list * );

#endif