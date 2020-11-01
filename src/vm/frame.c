#include "vm/frame.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "string.h"

struct hash ft;
struct lock ft_lock;
struct list evict_list;

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

void evict_init(void){
    list_init(&evict_list);
}

void ft_init(void){
    lock_init(&ft_lock);
    lock_acquire(&ft_lock);
    hash_init(&ft,ft_hash_func,ft_less,NULL);
    lock_release(&ft_lock);
}

struct fte* frame_alloc(struct spte* spte, enum palloc_flags flags){
    lock_acquire(&ft_lock);
    // printf("spte file : %p upage: %p\n",spte->file, spte->upage);
    void *kpage = palloc_get_page(flags);
    if(kpage == NULL){
        void * victim = find_evict();
        kpage = victim;
        spte->swap_index = swap_out(victim);
    }
    
    struct fte* fte = (struct fte*)malloc(sizeof(struct fte));
    if(!fte){
        lock_release(&ft_lock);
        return NULL;
    }

    fte->t = thread_current();
    fte->kpage = kpage;
    fte->spte = spte;
    list_push_back(&evict_list,&fte->evict_elem);
    hash_insert(&ft,&fte->elem);
    if(spte->file == NULL){
        lock_release(&ft_lock);
        pagedir_set_page(fte->t->pagedir,spte->upage,kpage,true);
        return fte;
    }

    if(spte->status == VM_EXEC_FILE){
        return frame_alloc_exec(spte,flags,fte);
    }
    else if(spte->status == VM_SWAP_DISK){
        return frame_alloc_swap(spte,flags,fte);
    }   
}

struct fte* frame_alloc_exec(struct spte* spte, enum palloc_flags flags,struct fte* fte){
    void* kpage = fte->kpage;
    file_seek(spte->file,spte->ofs);
    /* Load this page. */
    if (spte->read_bytes > 0 && file_read_at(spte->file, kpage, spte->read_bytes, spte->ofs) != (int) spte->read_bytes)
        {   
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

struct fte* frame_alloc_swap(struct spte* spte, enum palloc_flags flags,struct fte* fte){
    swap_in(spte->swap_index,fte->kpage);
    spte->status = VM_EXEC_FILE;
    pagedir_set_page(fte->t->pagedir,spte->upage,fte->kpage,spte->writable);
    lock_release(&ft_lock);
    return fte;
}


void * find_evict(){
    struct list_elem *e;
    for(e=list_front(&evict_list);;e=list_next(e)){
        if(e==list_tail(&evict_list))
            e = list_head(&evict_list);

        struct fte* temp  = list_entry(e,struct fte,evict_elem);
        if(pagedir_is_accessed(temp->t->pagedir,temp->spte->upage))
            pagedir_set_accessed(temp->t->pagedir,temp->spte->upage,false);
        else{
            list_remove(e);
            hash_delete(&ft,&temp->elem);
            temp->spte->status = VM_SWAP_DISK;
            void * empty_kpage = temp ->kpage;
            free(temp);
            return empty_kpage;
        }
    }
}