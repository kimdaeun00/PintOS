#include <debug.h>
#include "threads/thread.h"
#include "hash.h"


enum vm_status
{
    VM_ALL_ZERO,
    VM_SWAP_DISK,
    VM_EXEC_FILE,
    VM_ON_MEMORY
};

struct spte 
{
    void * upage;
    struct file *file;
    uint32_t ofs;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
    int status;
    struct hash_elem elem;
};

struct spt
{
    struct hash * table;
};


unsigned spt_hash_func(const struct hash_elem *, void *);
bool spt_less(const struct hash_elem *, const struct hash_elem *, void *);
struct spt* spt_init(void);
struct spte* spte_init(void *, struct file *, uint32_t, uint32_t, uint32_t , bool);
struct spte* spt_get_spte(void *);