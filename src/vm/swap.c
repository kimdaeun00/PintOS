#include "vm/swap.h"
#include "kernel/bitmap.h"
#include "threads/synch.h"
#include "devices/block.h"
#include "threads/thread.h"
#include "vm/page.h"
#include "threads/vaddr.h"

struct bitmap * st;
struct lock st_lock;
struct block *swap_disk;

// bitmap true가 비어있는거임

void swap_init(void){
    lock_init(&st_lock);
    swap_disk = block_get_role(BLOCK_SWAP);
    st = bitmap_create(block_size(swap_disk));
    bitmap_set_all(st,true);
}

void swap_in(block_sector_t swap_index, void *kpage){
    lock_acquire(&st_lock);
    // printf("swap in start %d\n",thread_current()->tid);
    if(bitmap_test(st,swap_index)){
        lock_release(&st_lock);
        exit(-1);
    }
    bitmap_flip(st,swap_index);
    for(int i=0;i<8;i++){
        block_read(swap_disk, i+ 8*swap_index, kpage+i*BLOCK_SECTOR_SIZE);
    }
    
    // printf("swap in end\n");
    lock_release(&st_lock);
}

block_sector_t swap_out(void * kpage){
    lock_acquire(&st_lock);
    // printf("swap out start %d\n",thread_current()->tid);
    block_sector_t available = bitmap_scan_and_flip(st,0,1,true);
    if(available == BITMAP_ERROR){
        lock_release(&st_lock);
        exit(-1);
    }
    for(int i=0;i<8;i++){
        block_write(swap_disk, i+ 8*available, kpage+i*BLOCK_SECTOR_SIZE);
    }
    // printf("swap out end\n");
    lock_release(&st_lock);
    return available;
}

void swap_remove(block_sector_t idx){
    lock_acquire(&st_lock);
    bitmap_set(st,idx,true);
    lock_release(&st_lock);
}