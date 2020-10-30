#include "vm/frame.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "vm/page.h"


struct hash ft;
struct lock ft_lock;

unsigned ft_hash_func(const struct hash_elem *temp, void *aux){
     return hash_int((int)hash_entry(temp, struct fte, elem) -> kpage);
}
 
bool ft_less(const struct hash_elem *a, const struct hash_elem *b, void *aux){
  int64_t upage_a = hash_entry(a, struct fte, elem) -> kpage;
  int64_t upage_b = hash_entry(b, struct fte, elem) -> kpage;
  if(upage_a > upage_b){
    return true;
  }
  else{
    return false;
  }
}

void ft_init(void){
    lock_init(&ft_lock);
    lock_acquire(&ft_lock);
    hash_init(&ft,ft_hash_func,ft_less,NULL);
    lock_release(&ft_lock);
}

struct fte* frame_alloc(struct spte* spte, enum palloc_flags flags){
    if(spte->status == VM_EXEC_FILE){
        return frame_alloc_exec(spte,flags);
    }
    else if(spte->status == VM_SWAP_DISK){
        return frame_alloc_swap(spte,flags);
    }
    
}

struct fte* frame_alloc_exec(struct spte* spte, enum palloc_flags flags){
    lock_acquire(&ft_lock);
    void *kpage = palloc_get_page(flags);
    if(kpage == NULL){
        //evict
    }
    
    struct fte* fte = (struct fte*)malloc(sizeof(struct fte));
    if(!fte){
        return NULL;
    }

    fte->t = thread_current();
    fte->kpage = kpage;
    fte->spte = spte;
    hash_insert(&ft,&fte->elem);
    file_seek(spte->file,spte->ofs);
    /* Load this page. */
    if (spte->read_bytes > 0 && file_read_at(spte->file, kpage, spte->read_bytes, spte->ofs) != (int) spte->read_bytes)
        {   
            printf("zz\n");
            lock_release(&ft_lock);
            palloc_free_page (kpage);
            free(fte);
            return NULL; 
        }    
    
    memset (kpage + spte->read_bytes, 0, spte->zero_bytes);

    /* Add the page to the process's address space. */
    if (!pagedir_set_page (fte->t->pagedir, spte->upage, kpage, spte->writable)) 
    {
        lock_release(&ft_lock);
        palloc_free_page (kpage);
        free(fte);
        return NULL; 
    }
    lock_release(&ft_lock);
    return fte;
}

struct fte* frame_alloc_swap(struct spte* spte, enum palloc_flags flags){
    struct fte* fte = (struct fte*)malloc(sizeof(struct fte));
    return fte;
}
