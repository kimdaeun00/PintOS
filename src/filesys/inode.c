#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define SECTOR_CNT BLOCK_SECTOR_SIZE/4
#define DIRECT_CNT SECTOR_CNT - 4
/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t direct_sector[DIRECT_CNT];
    block_sector_t indirect_sector;
    block_sector_t double_indirect_sector;
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
  };

struct indirect_block{
  block_sector_t sectors[SECTOR_CNT];
};

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);
  if (pos < inode->data.length)
    return find_block(&inode->data, pos / BLOCK_SECTOR_SIZE);
  else
    return -1;
}

block_sector_t find_block(const struct inode_disk* disk,size_t index){
  size_t cnt = index;
  if(cnt < DIRECT_CNT ){
    return disk->direct_sector[cnt];
  }
  cnt -= DIRECT_CNT;

  if(cnt < SECTOR_CNT){
    struct indirect_block * indirect_block = calloc(1,sizeof(struct indirect_block));
    cache_read(fs_device,disk->indirect_sector,indirect_block);
    block_sector_t result = indirect_block->sectors[cnt];
    free(indirect_block);
    return result;
  }
  cnt -= SECTOR_CNT;

  if(cnt < SECTOR_CNT*SECTOR_CNT){
    size_t first_index = cnt/SECTOR_CNT;
    size_t second_index = cnt%SECTOR_CNT;
    struct indirect_block * indirect_block1 = calloc(1,sizeof(struct indirect_block));
    cache_read(fs_device,disk->double_indirect_sector,indirect_block1);
    
    struct indirect_block * indirect_block2 = calloc(1,sizeof(struct indirect_block));
    cache_read(fs_device,indirect_block1->sectors[first_index],indirect_block2);
    
    block_sector_t result = indirect_block2->sectors[second_index];
    free(indirect_block1);
    free(indirect_block2);
    return result;
  }
  return -1; //fail
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

size_t iterate_alloc(block_sector_t * sectors, off_t length, size_t size ){
  static char empty[BLOCK_SECTOR_SIZE] = {0,};
  size_t cnt = 0;
  for(int i =0 ; i<length; i ++){
    if(cnt == size)  
      break;

    if(!sectors[i]){
      if (!free_map_allocate (1,&sectors[i]))
        return cnt; 
      cache_write(fs_device,sectors[i],empty);
      cnt ++;
    }
  }
  return cnt; 
}

size_t inode_alloc_indirect(block_sector_t * sectors, size_t size){
  static char empty[BLOCK_SECTOR_SIZE] = {0,};
  struct indirect_block *indirect_block = (struct indirect_block*) malloc(sizeof(struct indirect_block));
  if(*sectors == 0){ 
    free_map_allocate(1,sectors);
    cache_write(fs_device,*sectors,empty);
  }

  cache_read(fs_device,*sectors,indirect_block);
  size_t cnt = iterate_alloc(indirect_block->sectors,SECTOR_CNT,size);
  cache_write(fs_device,*sectors,indirect_block);
  return cnt;
}
size_t inode_alloc_double(block_sector_t * sectors, size_t size){
  static char empty[BLOCK_SECTOR_SIZE] = {0,};
  struct indirect_block *doubly_blocks= (struct indirect_block*) malloc(sizeof(struct indirect_block)); 
  size_t cnt = size;
  if(*sectors == 0){ 
    if(!free_map_allocate(1,sectors)){
      return size;
    }
    cache_write(fs_device,*sectors,empty);
  }
  cache_read(fs_device,*sectors,doubly_blocks);

  //1st level allocation
  for(int i =0 ; i<SECTOR_CNT; i ++){
    if(!doubly_blocks->sectors[i]){
      if (!free_map_allocate (1,&doubly_blocks->sectors[i]))
        return size - cnt;
      cache_write(fs_device,doubly_blocks->sectors[i],empty);
    }
  }
  
  //second level allocation
  for(int i=0; i<SECTOR_CNT ; i++){
    cnt -= inode_alloc_indirect(doubly_blocks->sectors[i],cnt);
  }

  cache_write(fs_device,*sectors,doubly_blocks);
  return size-cnt;  

}

size_t inode_alloc(struct inode_disk* disk, size_t sectors){
  size_t cnt = sectors;
  //direct
  cnt -= iterate_alloc(disk->direct_sector,DIRECT_CNT,cnt);
  if(cnt ==0)
    return sectors;
  //indirect
  cnt -=inode_alloc_indirect(&disk->indirect_sector,cnt);
  if(cnt ==0)
    return sectors;
  //doubly indirect
  cnt -= inode_alloc_double(disk->double_indirect_sector,cnt);

  ASSERT(cnt == 0);
  return sectors - cnt;

}

size_t iterate_free(block_sector_t * sectors, off_t length, size_t size ){
  size_t cnt = 0;
  for(int i =0 ; i<length; i ++){
    if(cnt == size)  
      break;

    if(sectors[i]){
      free_map_release(sectors[i],1);
      cnt ++;
    }
  }
  return cnt; //length-cnt; 
}

size_t inode_free_indirect(block_sector_t *sectors, size_t size){
  size_t cnt;
  struct indirect_block *indirect_block;
  if(*sectors != 0){
    cache_read(fs_device,*sectors,indirect_block);
    cnt = iterate_free(indirect_block->sectors,SECTOR_CNT,size);
  }
  cache_write(fs_device,*sectors,indirect_block);
  return cnt;
  
}

size_t inode_free_double(block_sector_t *sectors, size_t size){
  struct indirect_block *doubly_block;
  size_t cnt = size;
  if(*sectors != 0){
    cache_read(fs_device,*sectors,doubly_block);
    for(int i = 0; i<SECTOR_CNT; i++){
      cnt -= inode_free_indirect(doubly_block->sectors[i],cnt);
    }
    cache_write(fs_device,*sectors,doubly_block);
  }
  return size - cnt;
}

void inode_free(struct inode_disk* disk){
  size_t sectors = bytes_to_sectors(disk->length);

  //direct
  sectors -= iterate_free(disk->direct_sector,DIRECT_CNT,sectors);
  if(sectors == 0)
    return ;

  //indirect
  sectors -= inode_free_indirect(disk->indirect_sector,sectors);
  if(sectors == 0)
    return ;

  //doubly-indirect
  sectors -= inode_free_double(disk->double_indirect_sector,sectors);

}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;
  ASSERT (length >= 0);
  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);
  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      size_t alloc_sector = inode_alloc(disk_inode,sectors);
      if(alloc_sector == sectors){
        cache_write (fs_device, sector, disk_inode);
        success = true;
      } 
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;
  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  cache_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
          inode_free(&inode->data);
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;
  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;
      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;
      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          cache_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          cache_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;
  if (inode->deny_write_cnt)
    return 0;

  //extend file
  

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          cache_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            cache_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          cache_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}
