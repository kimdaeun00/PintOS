#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "hash.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "userprog/syscall.h"

unsigned spt_hash_func(const struct hash_elem *temp, void *aux){
     return hash_int((int)hash_entry(temp, struct spte, elem) -> upage);
}

void spt_hash_destroy(struct hash_elem *e, void *spt){
    struct spte *spte = hash_entry(e, struct spte, elem);
    if(spte->status == VM_ON_MEMORY ){
      struct fte* fte = spte_to_fte(spte);
      if(fte){
        fte_destroy(fte);
      }
      else{
        printf("%d\n",spte->status);
        PANIC("has fte but not in ft");
      }
    }
    else if(spte->status == VM_SWAP_DISK){
      swap_remove(spte->swap_index);
    }
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

void mmape_init(int mapid, void *addr, void * file_addr, struct file* file,struct spte* spte){
    struct mmape* mmape = (struct mmape*)malloc(sizeof(struct mmape));
    mmape->mapid = mapid;
    mmape->addr = addr;
    mmape->file_addr = file_addr;
    mmape->file = file;
    mmape->spte = spte;
    list_push_back(&thread_current()->mmap_list,&mmape->elem);
}

void mmape_exit(struct list * mmap_list){
  struct list_elem *temp;
  struct mmape * mmape;
  if(list_empty(mmap_list))
  {
    return;
  }
  for(temp = list_front(mmap_list);temp->next != NULL; temp = list_next(temp)){
    mmape = list_entry(temp,struct mmape, elem);
    munmap(mmape->mapid);
    // free(mmape);
  }
}