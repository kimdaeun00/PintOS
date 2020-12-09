#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include "filesys/file.h"

struct bitmap;
struct inode_disk;

void inode_init (void);
/* added */
block_sector_t find_block(const struct inode_disk*, size_t);
size_t iterate_alloc(block_sector_t *, off_t, size_t);
size_t inode_alloc_indirect(block_sector_t *, size_t);
size_t inode_alloc_double(block_sector_t *, size_t);
size_t inode_alloc(struct inode_disk*, size_t);
void inode_free(struct inode_disk*);
size_t iterate_free(block_sector_t *, off_t, size_t);
size_t inode_free_indirect(block_sector_t*, size_t);
size_t inode_free_double(block_sector_t*, size_t);

bool inode_create (block_sector_t, off_t,int);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

bool file_is_dir(struct file*);
bool inode_is_dir(struct inode*);
block_sector_t inode_to_inum(struct file*);
bool inode_is_open(struct inode* );

#endif /* filesys/inode.h */
