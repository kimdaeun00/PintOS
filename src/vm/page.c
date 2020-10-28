#include "vm/page.h"
#include "threads/thread.h"
#include "hash.h"


unsigned spt_hash_func(const struct hash_elem *temp, void *aux){
    return hash_int((int64_t)hash_entry(temp, struct spte, elem) -> upage);
}

bool spt_less(const struct hash_elem *a, const struct hash_elem *b, void *aux){
  int64_t upage_a = hash_entry(a, struct spte, elem) -> upage;
  int64_t upage_b = hash_entry(b, struct spte, elem) -> upage;
  if(upage_a > upage_b){
    return true;
  }
  else{
    return false;
  }
}


void spt_init(struct spt* spt){
    hash_init(spt->table,spt_hash_func,spt_less,NULL);

}

struct spte* spte_init(void *upage, struct file *file, uint32_t ofs, uint32_t read_bytes, uint32_t zero_bytes){
    struct spte* spte = (struct spte*)malloc(sizeof(struct spte));
    spte->upage =  upage;
    spte->status = VM_EXEC_FILE;
    spte->file = file;
    spte->file = ofs;
    spte->file = read_bytes;
    spte->file = zero_bytes;
    hash_insert(thread_current()->spt->table,&spte->elem);
    return spte;
}

struct spte* spt_get_spte(void *addr){
    struct spt* spt = thread_current()->spt;
    struct spte* temp;
    temp->upage = addr;
    return hash_entry(hash_find(spt->table,&temp->elem),struct spte,elem);
}