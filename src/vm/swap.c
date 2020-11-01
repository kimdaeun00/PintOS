#include "vm/swap.h"
#include "kernel/bitmap.h"
#include "threads/synch.h"
#include "devices/block.h"
#include "vm/page.h"

struct bitmap * st;
struct lock *st_lock;
struct block *swap_disk;

void st_init(){
    lock_init(st_lock);
    swap_disk = block_get_role(BLOCK_SWAP);
    st = bitmap_create(block_size(swap_disk));
    bitmap_set_all(st,true);
}

void swap_in(block_sector_t swap_index, void *kpage){
    lock_acquire(st_lock);
    if(!bitmap_test(st,swap_index)){
        lock_release(st_lock);
        exit(-1);
    }

    for(int i=0;i<8;i++){
        block_read(swap_disk,i+swap_index,kpage);
    }
    bitmap_set(st,swap_index,true);
    lock_release(st_lock);

}

block_sector_t swap_out(void * kpage){
    lock_acquire(st_lock);
    block_sector_t available = bitmap_scan_and_flip(st,0,8,false);
    if(available == BITMAP_ERROR){
        lock_release(st_lock);
        exit(-1);
    }
    for(int i=0;i<8;i++){
        block_write(swap_disk,i+available,(uint8_t *)kpage+ i*BLOCK_SECTOR_SIZE);
    }
    lock_release(st_lock);
    bitmap_set(st,available,false);
    return available;
}
