#include "vm/frame.h"
#include "vm/page.h"
#include "userprog/pagedir.h"

static
void frame_init(struct ft* ft){
    list_init(ft->table);
}

static
struct fte* find_fte(struct spte* spte){
    struct list_elem* e;
    for (e = list_begin (ft->table); e != list_end (ft->table);
       e = list_next (e))
    {
        if(list_entry(e,struct fte, elem)->spte->upage == spte->upage){
            return list_entry(e,struct fte, elem);
        }
    }
    return NULL;
}

static
struct fte* find_evict(void){
    struct list_elem *e;
    for(e = list_begin(ft->table); e != list_end(ft->table); e = list_next(e)){
        struct fte* target_fte = list_entry(e,struct fte,elem);
        if(pagedir_is_accessed(target_fte->thread->pagedir,target_fte->spte->upage))
            return target_fte;
    }
}

void *frame_alloc(enum palloc_flags flags, struct spte* spte){
    if(spte->status == VM_EXEC_FILE ){
        struct fte * fte = (struct fte*)malloc(sizeof(struct fte));
        void *kpage = palloc_get_page(flags);
        fte->spte = spte;
        spte->status = VM_ON_MEMORY;
        fte->thread = thread_current();

        if(!kpage){
            struct fte* evict_fte = find_evict();
            fte->kpage = evict_fte->kpage;
        }
        else{
            fte->kpage = kpage;
            }
        list_push_back(ft->table,&fte->elem);

        if (file_read (spte->file, fte->kpage, spte->read_bytes) != (int) spte->read_bytes)
            {
            palloc_free_page (kpage);
            return false; 
            }
        memset (fte->kpage + spte->read_bytes, 0, spte->zero_bytes); 

        pagedir_set_page (fte->thread->pagedir, spte->upage, kpage, spte->writable);
        return fte->kpage;
    }

    else if(spte->status == VM_SWAP_DISK ){

    }

}