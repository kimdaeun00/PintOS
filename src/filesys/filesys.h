#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include <filesys/file.h>
#include <list.h>
#include "filesys/off_t.h"
#include "threads/synch.h"

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

/* Block device that contains the file system. */
extern struct block *fs_device;
struct lock filesys_lock;

struct file_descriptor
{
  int fd;
  struct file* file;
  struct list_elem elem;
  struct dir* dir;
};



void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size, int dir);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);

void split_name(char*, char*, char*);
char * contin_slash(char * );
struct dir * open_directories(char*);

#endif /* filesys/filesys.h */
