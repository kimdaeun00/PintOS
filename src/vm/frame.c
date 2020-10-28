#include "vm/frame.h"
#include "vm/page.h"

static
void frame_init(struct ft* ft){
    list_init(ft->table);
}

static
struct fte* find_fte(struct spte* spte){
    for (e = list_begin (ft->table); e != list_end (ft->table);
       e = list_next (e))
    {
        if(list_entry(e,struct fte, elem)->spte->upage == spte->upage){
            return list_entry(e,struct fte, elem);
        }
    }
    return NULL;
}

void *frame_alloc(enum palloc_flags flags, struct spte* spte){
    if(spte->status == VM_EXEC_FILE ){
        struct fte * fte = (struct fte*)malloc(sizeof(struct fte));
        void *kpage = palloc_get_page(flags);
        if(!kpage){
            //evit page
        }
        fte->kpage = kpage;
        list_insert_ordered(ft->table,fte->elem,less_func,NULL);
    }

    else if(spte->status == VM_SWAP_DISK ){

    }

}