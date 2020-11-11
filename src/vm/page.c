#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "hash.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "vm/frame.h"
#include "vm/swap.h"

unsigned spt_hash_func(const struct hash_elem *temp, void *aux){
     return hash_int((int)hash_entry(temp, struct spte, elem) -> upage);
}

void spt_hash_destroy(struct hash_elem *e, void *spt){
    struct spte *spte = hash_entry(e, struct spte, elem);
    if(spte->status == VM_ON_MEMORY){
      struct fte* fte = spte_to_fte(spte);
      if(fte)
        fte_destroy(fte);
      else{
        // printf("no fte but exec %p\n",spte);
        exit(-1);
      }
    }
    else if(spte->status == VM_SWAP_DISK){
      swap_remove(spte->swap_index);
    }
    // hash_delete(spt,e);
    free(spte);

}
 
bool spt_less(const struct hash_elem *a, const struct hash_elem *b, void *aux){
  int64_t upage_a = hash_entry(a, struct spte, elem) -> upage;
  int64_t upage_b = hash_entry(b, struct spte, elem) -> upage;
  if(upage_a < upage_b){
    return true;
  }
  else{
    return false;
  }
}

struct spte* spte_init(void * upage, enum vm_status state,struct file *file, uint32_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable){
    // printf("spte upage: %p file: %p ofs: %p r: %p z: %p\n",upage, file, ofs, read_bytes, zero_bytes);
    struct spte* spte = (struct spte *)malloc(sizeof(struct spte));
    spte->upage = upage;
    spte->status = state;
    spte->file = file;
    spte->ofs = ofs;
    spte->read_bytes = read_bytes;
    spte->zero_bytes = zero_bytes;
    spte->writable = writable;
    spte->swap_index = NULL;
    spte->dirty_bit = false;
    hash_insert(&thread_current()->spt,&spte->elem);
    return spte;
}

struct spte* spt_get_spte(void *addr){
    struct hash spt = thread_current()->spt;
    struct spte* temp = (struct spte*)malloc(sizeof(struct spte));
    temp->upage = pg_round_down(addr);
    struct spte* result = hash_entry(hash_find(&spt,&temp->elem),struct spte,elem);
    free(temp);
    return result;
 } 
