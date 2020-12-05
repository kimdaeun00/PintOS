#include "filesys/cache.h"
#include "lib/kernel/list.h"
#include "threads/malloc.h"
#include "lib/string.h"

struct lock cache_lock;

void cache_init(void){
    list_init(&cache_list);
    lock_init(&cache_lock);
}

struct cache_entry *  is_hit(block_sector_t idx){
    struct cache_entry *target = NULL;
    struct list_elem *e;
    if(list_empty(&cache_list))
        return NULL;

    for(e = list_front(&cache_list);e->next != NULL ; e = list_next(e)){
        struct cache_entry * temp = list_entry(e,struct cache_entry, elem);
        if(temp->block_idx == idx)
            target = temp;
    }
    return target;
}

struct list_elem* clock_next(struct list* l,struct list_elem *e){
    if(list_back(l) == e){
        return list_front(l);
    }
    else{
        return list_next(e);
    }
}

void cache_write_back(struct cache_entry * ce){
    if(ce->dirty){
        block_write(ce->block,ce->block_idx,ce->buffer);
    }
}


void cache_evict(void){
    ASSERT(list_size(&cache_list)==64);
    //clock
    struct list_elem *e;
    for(e = list_front(&cache_list);e->next != NULL ; e = list_next(e)){
        struct cache_entry * temp = list_entry(e,struct cache_entry, elem);
        if(temp->is_start)
            break;
    }
    
    for(;; e = clock_next(&cache_list,e)){
        struct cache_entry * temp = list_entry(e,struct cache_entry, elem);
        if(temp->accessed){
            // printf("evict : %d\n",temp->block_idx);
            cache_write_back(temp);
            struct cache_entry * next = list_entry(clock_next(&cache_list,e),struct cache_entry, elem);
            next->is_start = true;
            list_remove(e);
            free(temp);
            break;
        }
        else{
            temp->accessed = true;
        }
    }
}


struct cache_entry * set_cache(struct block *block, block_sector_t sector){
    struct cache_entry *cache;
    // printf("set cache : %d\n",sector);
    if(list_size(&cache_list)==64){ // eviction required
        cache_evict();
    }
    ASSERT(list_size(&cache_list)!=64);
    cache = (struct cache_entry *)malloc(sizeof(struct cache_entry));
    cache->accessed = false;
    cache->block_idx = sector;
    cache->dirty = false;
    cache->is_start = list_empty(&cache_list);
    cache->block = block;
    block_read(block,sector,cache->buffer);
    list_push_back(&cache_list,&cache->elem);
    return cache;
}

/* write BUFFER into BLOCK SECTOR's cache. */
void cache_write(struct block *block, block_sector_t sector, const void *buffer){
    lock_acquire(&cache_lock);
    // printf("cache write : %d\n",sector);
    struct cache_entry *target = is_hit(sector);
    if(target == NULL){ //not - HIT
        target = set_cache(block, sector);
    }
    memcpy(target->buffer,buffer,BLOCK_SECTOR_SIZE);
    target->dirty = true;
    lock_release(&cache_lock);
}

/* read from BLOCK SECTOR's cache into BUFFER */
void cache_read(struct block *block, block_sector_t sector, void *buffer){
    lock_acquire(&cache_lock);
    struct cache_entry *target = is_hit(sector);
    if(target == NULL){ //not - HIT
        target = set_cache(block, sector);
    }
    memcpy(buffer,target->buffer,BLOCK_SECTOR_SIZE);
    lock_release(&cache_lock);
}

void cache_exit(void){
    lock_acquire(&cache_lock);
    struct list_elem *e;
    for(e = list_front(&cache_list);e->next != NULL ; e = list_next(e)){
        struct cache_entry * temp = list_entry(e,struct cache_entry, elem);
        cache_write_back(temp);
        // free(temp);
    }
    lock_release(&cache_lock);
}