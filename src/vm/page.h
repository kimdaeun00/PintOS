#include <debug.h>

enum vm_status
{
    VM_SWAP_DISK,
    VM_EXEC_FILE,
    VM_READABLE_FILE
};

struct spt
{
    struct hash * table;
};

struct spte 
{
    void * upage;
    struct file *file;
    uint32_t ofs;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    int status;
    struct hash_elem elem;
};