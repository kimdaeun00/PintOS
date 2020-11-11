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

struct list ft;
struct lock ft_lock;


void ft_init(void){
    lock_init(&ft_lock);
    lock_acquire(&ft_lock);
    list_init(&ft);
    lock_release(&ft_lock);
}

struct fte* spte_to_fte(struct spte* spte){
    struct list_elem *e;
    for(e=list_front(&ft);e->next != NULL;e=list_next(e)){
        struct fte* fte = list_entry(e,struct fte,elem);
        if(fte->spte == spte){
            return fte;
        }
    }
    return NULL;
}

void fte_destroy(struct fte* fte){
    lock_acquire(&ft_lock);
    list_remove(&fte->elem);
    free(fte);
    lock_release(&ft_lock);

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
    // printf("install : %p\n",spte);
    struct fte* fte = (struct fte*)malloc(sizeof(struct fte));
    if(!fte){
        lock_release(&ft_lock);
        return NULL;
    }
    
    fte->t = thread_current();
    fte->kpage = kpage;
    fte->spte = spte;
    // fte->pinned = true;
    list_push_back(&ft,&fte->elem);
    lock_release(&ft_lock);
    return fte;
}
 
struct fte* frame_alloc(struct spte* spte, enum palloc_flags flags){
    void* kpage = get_kpage(flags);
    struct fte* fte = install_new_fte(kpage,spte);

    if(spte->status == VM_EXEC_FILE){
        return frame_alloc_exec(spte,flags,fte);
    }
    else if(spte->status == VM_SWAP_DISK){
        return frame_alloc_swap(spte,flags,fte);
    }
    else if(spte->status == VM_STK_GROW){
        memset(fte->kpage,0,PGSIZE);
        pagedir_set_page(fte->t->pagedir,spte->upage,fte->kpage,spte->writable);
        // fte->pinned = false;
    }
    return fte;
    
}

struct fte* frame_alloc_exec(struct spte* spte, enum palloc_flags flags,struct fte* fte){
    // printf("frame_alloc_exec %p %p %p\n",fte->t->pagedir,spte->upage, fte->kpage);
    // printf("f alloc spte : upage: %p, thread : %d\n", spte->upage,fte->t->tid);
    lock_acquire(&ft_lock);
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
    // printf("%p\n",kpage);
    if (!pagedir_set_page (fte->t->pagedir, spte->upage, kpage, spte->writable)) 
    {
        lock_release(&ft_lock);
        palloc_free_page (kpage);
        free(fte);
        return NULL; 
    }
    lock_release(&ft_lock);
    spte->status = VM_ON_MEMORY;
    // fte->pinned = false;
    return fte;
}

struct fte* frame_alloc_swap(struct spte* spte, enum palloc_flags flags,struct fte* fte){
    // printf("swap_in %p %p \n",spte->upage, fte->kpage);
    lock_acquire(&ft_lock);
    swap_in(spte->swap_index,fte->kpage);
    spte->status = VM_ON_MEMORY;
    pagedir_set_page(fte->t->pagedir,spte->upage,fte->kpage,spte->writable);
    lock_release(&ft_lock);
    fte->pinned = false;
    return fte;
}


void * find_evict(){
    ASSERT(!list_empty(&ft))

    struct list_elem *e;
    // bool detect = false;
    lock_acquire(&ft_lock);
    // printf("ft size : %d\n",list_size(&ft));
    /* frame table에서 pinned된 애들(read나 write될 애들)은 evict에서 제외시킴 */
    // struct fte* temp = NULL;
    // int i=0;
    // for(e=list_front(&ft); e->next != NULL ;e= list_next(e)){
    //     temp  = list_entry(e, struct fte, elem);
    //     if(!temp->pinned){
    //         detect = true;
    //         // printf("#%d\n",++i);    
    //         break;
    //     }
    // }
    
    // ASSERT(detect)

    e = list_pop_front(&ft);
    struct fte* temp  = list_entry(e,struct fte,elem);
    
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

void spt_exit(struct hash spt){
    hash_destroy(&spt,spt_hash_destroy);
}

/* kpage fte의 pinned를 pin으로 설정 */
void frame_alloc_file(void* buffer, unsigned size){
    unsigned bound = buffer + size;
    int i=0;
    for(void *temp = pg_round_down(buffer); temp<bound; temp+=PGSIZE){
        struct spte * spte = spt_get_spte(temp);
        if(!spte)
            exit(-1);
        spte->status = VM_EXEC_FILE;
        struct fte * fte = frame_alloc(spte,PAL_USER);
    }
}

void frame_unpin_file(void *buffer, unsigned size){
    unsigned bound = buffer + size;
    for(void *temp = pg_round_down(buffer); temp<bound; temp+=PGSIZE){
        struct spte * spte = spt_get_spte(temp);
        if(!spte)
            exit(-1);
        struct fte* fte = spte_to_fte(spte);
        fte->pinned = false;
    }
}
