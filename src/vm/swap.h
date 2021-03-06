#ifndef _SWAP_HEADER_H
#define _SWAP_HEADER_H

#include "devices/block.h"

#include <debug.h>

struct lock st_lock;

void swap_init(void);
void swap_in(block_sector_t, void *);
block_sector_t swap_out(void *);
void swap_remove(block_sector_t);

#endif