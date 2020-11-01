#ifndef _SWAP_HEADER_H
#define _SWAP_HEADER_H

#include "devices/block.h"

#include <debug.h>

void swap_init(void);
void swap_in(block_sector_t, void *);
block_sector_t swap_out(void *);

#endif