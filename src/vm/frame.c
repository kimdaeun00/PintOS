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
#include "threads/vaddr.h"
#include "userprog/syscall.h"

struct list ft;


void ft_init(void){
    lock_init(&ft_lock);
    lock_acquire(&ft_lock);
    list_init(&ft);
    lock_release(&ft_lock);
}

struct fte* spte_to_fte(struct spte* spte){
    struct list_elem *e;
    if(list_empty(&ft))
        return NULL; 
    for(e=list_front(&ft);e->next != NULL;e=list_next(e)){
        struct fte* fte = list_entry(e,struct fte,elem);
        if(fte->spte == spte){
            return fte;
        }
    }
    return NULL;
}

void fte_destroy(struct fte* fte){
    // lock_acquire(&ft_lock);
    list_remove(&fte->elem);
    free(fte);
    // lock_release(&ft_lock);

}

void *get_kpage(enum palloc_flags flags){
    void *kpage = palloc_get_page(flags);
    if(!kpage){
        kpage = find_evict();
    }
    return kpage; 
 }

struct fte* install_new_fte(void *kpage, struct spte* spte){
    // printf("new fte :  kpage : %p , upage:  %p, spte : %p, thread : %d       count : %d\n",kpage,spte->upage,spte,thread_current()->tid,list_size(&ft));
    lock_acquire(&ft_lock);
    struct fte* fte = (struct fte*)malloc(sizeof(struct fte));
    if(!fte){
        lock_release(&ft_lock);
        return NULL;
    }
    
    fte->t = thread_current();
    fte->kpage = kpage;
    fte->spte = spte;
    fte->inevictable = true;
    list_push_back(&ft,&fte->elem);
    lock_release(&ft_lock);
    return fte;
}
 
struct fte* frame_alloc(struct spte* spte, enum palloc_flags flags){
    void* kpage = get_kpage(flags);
    struct fte* fte = install_new_fte(kpage,spte);
    if(spte->status == VM_EXEC_FILE){
        fte =  frame_alloc_exec(spte,flags,fte);
    }
    else if(spte->status == VM_SWAP_DISK){
        fte =  frame_alloc_swap(spte,flags,fte);
    }
    else if(spte->status == VM_STK_GROW){
        lock_acquire(&ft_lock);
        memset(fte->kpage,0,PGSIZE);
        pagedir_set_page(fte->t->pagedir,spte->upage,fte->kpage,spte->writable);
        spte->status = VM_ON_MEMORY;
        lock_release(&ft_lock);
    }
    fte->inevictable = false;
    return fte;
    
}

struct fte* frame_alloc_exec(struct spte* spte, enum palloc_flags flags,struct fte* fte){
    // printf("frame_alloc_exec %p %p %p\n",fte->t->pagedir,spte->upage, fte->kpage);
    lock_acquire(&ft_lock);
    void* kpage = fte->kpage;

    file_seek(spte->file,spte->ofs);
    /* Load this page. */
    if (spte->read_bytes > 0 && file_read_at(spte->file, kpage, spte->read_bytes, spte->ofs) != (int) spte->read_bytes)
        {
            lock_release(&ft_lock);
            palloc_free_page (kpage);
            list_remove(fte);
            free(fte);
            return NULL; 
        } 
    
    memset (kpage + spte->read_bytes, 0, spte->zero_bytes);
    

    /* Add the page to the process's address space. */
    if (!pagedir_set_page (fte->t->pagedir, spte->upage, kpage, spte->writable)) 
    {
        lock_release(&ft_lock);
        palloc_free_page (kpage);
        list_remove(fte);
        free(fte);
        return NULL; 
    }

    spte->status = VM_ON_MEMORY;
    lock_release(&ft_lock);
    return fte;
}

struct fte* frame_alloc_swap(struct spte* spte, enum palloc_flags flags,struct fte* fte){
    lock_acquire(&ft_lock);
    swap_in(spte->swap_index,fte->kpage);
    spte->status = VM_ON_MEMORY;
    fte->inevictable = false;
    pagedir_set_page(fte->t->pagedir,spte->upage,fte->kpage,spte->writable);
    lock_release(&ft_lock);
    return fte;
}


void * find_evict(){
    ASSERT(!list_empty(&ft))

    struct list_elem *e;
    lock_acquire(&ft_lock);
    /* frame table에서 pinned된 애들(read나 write될 애들)은 evict에서 제외시킴 */
    struct fte* temp = NULL;
    int i=0;
    for(e=list_front(&ft); e->next != NULL ;e= list_next(e)){
        temp  = list_entry(e, struct fte, elem);
        if(!temp->inevictable){
            break;
        }
    }
    
    struct spte* spte = temp->spte;
    spte->status = VM_SWAP_DISK;
    list_remove(e);
    spte->swap_index = swap_out(temp->kpage);
    pagedir_clear_page(temp->t->pagedir,spte->upage);
    void * ret = temp->kpage; 

    free(temp);
    lock_release(&ft_lock);
    return ret;


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

void spt_exit(struct hash *spt){
    hash_destroy(spt,spt_hash_destroy);
}

