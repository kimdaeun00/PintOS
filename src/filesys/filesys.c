#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/cache.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();
  cache_init();
  lock_init(&filesys_lock);

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
  cache_exit();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, int is_dir) 
{
  lock_acquire(&filesys_lock);
  // printf("create : %s\n",name);
  block_sector_t inode_sector = 0;
  char * filename = (char*)calloc(1,128);
  char * dirname = (char*)calloc(1,128);
  char * newname = (char*)calloc(1,128);
  split_name(name, filename, dirname);
  if(is_dir && strlen(filename)==0){
    int i;
    for(i = strlen(name)-1; i>=0; i--){
      if(name[i]!='/')
        break;
    }
    strlcpy(newname, name, i+1);
    split_name(newname, filename, dirname);
  }

  if(!strcmp(filename,".") || !strcmp(filename,"..")){
    return false;
  }
  // printf("new: %s file: %s dir: %s\n",name, filename, dirname);
  if(strlen(filename)>14){
    free(filename);
    free(dirname);
    lock_release(&filesys_lock);
    return false;
  }
  // printf("open_dir : %s\n",dirname);
  struct dir* dir = open_directories(dirname);
  // printf("create : %p\n", dir);
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size,is_dir)
                  && dir_add (dir, filename, inode_sector, is_dir)); 
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  dir_close (dir);

  free(filename);
  free(dirname);
  lock_release(&filesys_lock);
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  lock_acquire(&filesys_lock);
  // printf("open : %s\n",name);
  char * filename = (char*)calloc(1,128);
  char * dirname = (char*)calloc(1,128);
  struct inode *inode = NULL;
  if(strlen(name)==0){
    free(filename);
    free(dirname);
    lock_release(&filesys_lock);  
    return NULL;
  }
  split_name(name, filename, dirname);
  // printf("open dirname : %s, filename : %s\n",dirname, filename);
  struct dir* dir = open_directories(dirname);
  if(dir == NULL){
    lock_release(&filesys_lock);
    return NULL;
  }
  // if(dir!=NULL){
  // }
  if(strlen(filename)==0){
    inode = dir_get_inode(dir);
  }
  else{
    dir_lookup (dir, filename, &inode);
    dir_close (dir);
  }
  // if(inode == NULL || inode_is_removed(inode))
  //   return NULL;
  free(filename);
  free(dirname);
  lock_release(&filesys_lock);
  // printf("open success : %s\n",name);
  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  lock_acquire(&filesys_lock);
  char * filename = (char*)calloc(1,128);
  char * dirname = (char*)calloc(1,128);
  split_name(name, filename, dirname);
  struct dir* dir = open_directories(dirname);
  if(dir == NULL){
    lock_release(&filesys_lock);
    return false;
  }

  bool success = dir_remove(dir,filename);
  dir_close (dir); 
  free(filename);
  free(dirname);
  // if(success)
  //   printf("remove success : %s\n",name);
  // else
  //   printf("remove failed : %s\n",name);
  lock_release(&filesys_lock);
  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}


void split_name(char* name, char* filename, char* dirname){
  int i;
  for(i=strlen(name)-1;i>=0;i--){
    if(name[i]=='/')
      break;
    }
  if(i==-1){ // name??? '/'??? ?????? ??? : filename??? ????????? ??? 
    strlcpy(filename,name,strlen(name)+1);
    return;
  }

  // if(i==0){
  //   memcpy(dirname,");
  //   // dirname[1] = "\0";
  // }
  // else{
  strlcpy(dirname,name,i+2);
  // }

  if(i!=strlen(name)-1){  // '/'???????????? filename
    strlcpy(filename,name+i+1,strlen(name)-i);
  }
  return NULL;
}

char * contin_slash(char * dirname){
  if(strlen(dirname) >1 )
    if(dirname[0]=='/' && dirname[1]=='/')
      return dirname+1;
  return dirname;
}


/* ?????? split?????? dirname??? ???????????? ?????? */
struct dir * open_directories(char* dirname){
  char * new_dirname =  contin_slash(dirname);
  char *token, save_ptr;
  char * temp = (char*)calloc(1,128);
  strlcpy(temp,new_dirname,strlen(new_dirname)+1);
  struct dir* abs_dir = NULL;
  struct dir* temp_dir = NULL;
  if(strlen(new_dirname)==0){ //file name??? ??????????????? ?????? directory return
    // return dir_open_root;
    if(thread_current()->dir != NULL){
      // printf(" cur dir : %p\n", thread_current()->dir);
      abs_dir = dir_reopen(thread_current()->dir);
      // printf(" absdir : %p\n", abs_dir);
      return abs_dir;
    }
    abs_dir = dir_open_root();
    // printf("root : %p\n",abs_dir);
    return abs_dir;
  }

  if(temp[0] == '/'){ // absolute path
    abs_dir = dir_open_root();
    // printf("%p\n",abs_dir);
  }
  else if(thread_current()->dir == NULL){ // current thread is main
    abs_dir = dir_open_root();
  }
  else{
    abs_dir = dir_reopen(thread_current()->dir);
  }

  int i =0;
  //dirname??? /??? ??????????????? dir_open?????? 
  for (token = strtok_r (temp, "/", &save_ptr); token != NULL;
        token = strtok_r (NULL, "/", &save_ptr))
    {
      if(i++ ==0 && strlen(token)==0){
        continue;
      }
      if(strlen(token)==0){
        break;
      }
      struct inode* temp_inode = NULL;
      bool temp_bool = dir_lookup(abs_dir,token,&temp_inode);
      if(!temp_bool){
        // printf("not found : %s\n",token);
        dir_close(abs_dir); 
        return NULL; //???????????? ?????? directory??? ?????? return null
      }
      else{
        temp_dir = dir_open(temp_inode);
        if(temp_dir == NULL){
            dir_close(abs_dir);
            return NULL;
        }
        // abs_dir = dir_open(temp_inode);
        dir_close(abs_dir);
        abs_dir = temp_dir;
        
      }
    }
  free(temp);
  return abs_dir;
}