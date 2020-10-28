#include <debug.h>
#include "lib/kernel/list.h"
#include "threads/palloc.h"

struct lock frame_table_lock;
struct ft *ft;

struct ft{
  struct list *table;
};

struct fte {
  void *kpage;
  struct spte *spte;//evict된 page의 spte/pte 업데이트에 필요
  //evict된 page의 pagedir에 접근, pte업데이트에 필요  
  struct thread *thread;
  struct list_elem elem;
};