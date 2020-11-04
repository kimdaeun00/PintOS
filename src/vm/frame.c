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

struct list ft;
struct lock ft_lock;
struct list evict_list;


void evict_init(void){
    list_init(&evict_list);
}

void ft_init(void){
    lock_init(&ft_lock);
    lock_acquire(&ft_lock);
    list_init(&ft);
    lock_release(&ft_lock);
}

struct fte* frame_alloc(struct spte* spte, enum palloc_flags flags){
    lock_acquire(&ft_lock);
    // printf("spte file : %p upage: %p\n",spte->file, spte->upage);
    void *kpage = palloc_get_page(flags);
    if(kpage == NULL){
        void * victim = find_evict();
        kpage = victim;
    }

    struct fte* fte = (struct fte*)malloc(sizeof(struct fte));
    if(!fte){
        lock_release(&ft_lock);
        return NULL;
    }
    fte->t = thread_current();
    fte->kpage = kpage;
    // printf("kpage : %p , uapge : %p\n",kpage,spte->upage);
    fte->spte = spte;
    list_push_back(&ft,&fte->elem);
    list_push_back(&evict_list,&fte->evict_elem);
    if(spte->file == NULL){ 
        lock_release(&ft_lock);
        printf("!\n");
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
    // printf("exec\n");
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
    printf("swap_in %p %p \n",spte->upage, fte->kpage);
    swap_in(spte->swap_index,fte->kpage);
    spte->status = VM_EXEC_FILE;
    pagedir_set_page(fte->t->pagedir,spte->upage,fte->kpage,spte->writable);
    lock_release(&ft_lock);
    return fte;
}


void * find_evict(){
    struct list_elem *e;
    if(list_empty(&evict_list))
        return NULL;
    
    e = list_front(&evict_list);
    // e = list_next(list_front(&evict_list));
    struct fte* temp  = list_entry(e,struct fte,evict_elem);
    struct spte* spte = temp->spte;
    list_remove(e);
    spte->status = VM_SWAP_DISK;
    printf("assertion 1 %p %p %p\n",temp->t->pagedir, spte->upage, temp->kpage);
    spte->swap_index = swap_out(temp->kpage);
    pagedir_clear_page(temp->t->pagedir,spte->upage);
    printf("assertion 2\n");
    list_remove(&temp->elem);
    return temp->kpage;


    // for(e=list_front(&evict_list);;e=list_next(e)){
    //     if(e==list_tail(&evict_list)){
    //         printf("come back to head \n");
    //         e = list_head(&evict_list);
    //         }

    //     struct fte* temp  = list_entry(e,struct fte,evict_elem);
    //     printf("%p\n",temp->spte->upage);
    //     if(pagedir_is_accessed(temp->t->pagedir,temp->spte->upage)){
    //         pagedir_set_accessed(temp->t->pagedir,temp->spte->upage,false);
    //     }
    //     else{
    //         printf("end of iteration : %p\n",temp->spte->upage);
    //         list_remove(e);
    //         hash_delete(&ft,&temp->elem);
    //         temp->spte->status = VM_SWAP_DISK;
    //         void * empty_kpage = temp ->kpage;
    //         free(temp);
    //         return empty_kpage;
    //     }
    // }
}

void spt_exit(tid_t tid){
    struct list_elem *e;
    for(e=list_front(&ft);e->next != NULL;e=list_next(e)){
        struct fte* fte = list_entry(e,struct fte,elem);
        if(fte->t->tid == tid){
            free(fte->spte);
            list_remove(&fte->elem);
            // free(fte);
        }
    }
}