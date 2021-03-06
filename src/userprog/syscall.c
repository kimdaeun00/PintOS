#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "threads/pte.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "filesys/directory.h"
#include "filesys/inode.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/inode.h"

static int
get_user (const uint8_t *uaddr) {
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

static struct file_descriptor *fd_to_fd(int fd)
{
  struct list *fd_list = &thread_current()->fd_list;
  struct list_elem *e;
  struct file_descriptor *target;
  if (list_empty(fd_list))
    return NULL;
  // print_list(fd_list);
  for (e = list_begin(fd_list); e != list_end(fd_list); e = list_next(e))
  {
    if (e->next == NULL)
    {
      return NULL;
    }
    target = list_entry(e, struct file_descriptor, elem);

    if (target->fd == fd)
    {
      return target;
    }
    // printf("tf : %d\n",e == list_tail(fd_list) );
  }
  return NULL;
}

static void is_valid_arg(void* p){
  if (!is_user_vaddr(p)){
    exit(-1);
  }
}

static bool is_userspace(int *esp, int n)
{
  
  int *temp;
  
  for (int i = 0; i < n + 1; i++)
  {
    temp = esp + i + 1;
    if (!is_user_vaddr((void *)(temp))  || !is_user_vaddr((void *)(((char *)(temp)) + 3))) //|| get_user(temp) == -1
    {
      return false;
    }
  }
  return true;
}

static void syscall_handler(struct intr_frame *);

void syscall_init(void)
{
  lock_init(&sys_lock);
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f)
{
  int *esp = (int *)(f->esp);
  thread_current()->esp = esp;

  if(!is_userspace(esp,0)){
    exit(-1);
  }

  switch (*esp)
  {
  case SYS_HALT:

    if (!is_userspace(esp, 0))
    {
      exit(-1);
    }
    halt();
    break;

  case SYS_EXIT:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    exit(*(esp + 1));
    break;

  case SYS_EXEC:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    f->eax = exec((char *)(*(esp+1)));
    break;

  case SYS_WAIT:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    f->eax = wait(*(tid_t*)(esp+1));
    break;

  case SYS_CREATE:
    if (!is_userspace(esp, 2))
    {
      exit(-1);
    }
    f->eax = create((const char *)(*(esp + 1)), *(unsigned *)(esp + 2));
    break;

  case SYS_REMOVE:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    f->eax = remove((const char *)(*(esp + 1)));
    break;

  case SYS_OPEN:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    f->eax = open((const char *)(*(esp + 1)));
    break;

  case SYS_FILESIZE:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    f->eax = filesize(*(int *)(esp + 1));
    break;

  case SYS_READ:
    if (!is_userspace(esp, 3))
    {
      exit(-1);
    }
    f->eax = read(*(int *)(esp + 1), (void *)(*(esp + 2)), *(unsigned *)(esp + 3));
    break;

  case SYS_WRITE:
    if (!is_userspace(esp, 3))
    {
      exit(-1);
    }

    f->eax = write(*(int *)(esp + 1), (const char *)(*(esp + 2)), *(unsigned *)(esp + 3));
    break;

  case SYS_SEEK:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    seek(*(int *)(esp + 1), *(unsigned int *)(esp + 2));
    break;

  case SYS_TELL:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    f->eax = tell(*(int *)(esp + 1));
    break;

  case SYS_CLOSE:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    close(*(int *)(esp + 1));
    break;

  case SYS_MMAP:
    if (!is_userspace(esp, 2))
    {
      exit(-1);
    }
    f->eax = mmap(*(int *)(esp + 1), (void *)(*(esp + 2)));
    break;
  
  case SYS_MUNMAP:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    munmap(*(int *)(esp + 1));
    break;

  case SYS_CHDIR:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    f->eax = chdir((const char *)(*(esp + 1)));
    break;

  case SYS_MKDIR:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    f->eax = mkdir((const char *)(*(esp + 1)));
    break;

  case SYS_READDIR:
    if (!is_userspace(esp, 2))
    {
      exit(-1);
    }
    f->eax = readdir(*(int *)(esp + 1), (char *)(*(esp + 2)));
    break;

  case SYS_ISDIR:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    f->eax = isdir(*(int *)(esp + 1));
    break;

  case SYS_INUMBER:
    if (!is_userspace(esp, 1))
    {
      exit(-1);
    }
    f->eax = inumber(*(int *)(esp + 1));
    break; 
           
  default:
    break;
  }
}

void halt(void)
{
  shutdown_power_off();
}

void exit(int status)
{
  thread_current()->exit_status = status;
  thread_exit();
}

tid_t exec(const char *cmd_line)
{
  is_valid_arg(cmd_line);
  tid_t result;
  lock_acquire(&sys_lock);
  result = process_execute(cmd_line);
  lock_release(&sys_lock);
  return result;
}


int wait(tid_t pid)
{
  return process_wait(pid);
}


bool create(const char *file, unsigned initial_size)
{
  bool result;
  if (file == NULL)
  {
    exit(-1);
  }
  is_valid_arg(file);
  lock_acquire(&sys_lock);
  result = filesys_create(file, initial_size,0);
  lock_release(&sys_lock);
  return result;
}

bool remove(const char *file)
{
  is_valid_arg(file); 
  lock_acquire(&sys_lock);
  bool result = filesys_remove(file);
  lock_release(&sys_lock);
  return result;
}

int open(const char *file)
{
  if (file == NULL)
  {
    exit(-1);
  }
  is_valid_arg(file);
  lock_acquire(&sys_lock);
  struct file *open_file = filesys_open(file);
  if (open_file == NULL)
  {
    lock_release(&sys_lock);
    return -1;
  }

  int new_fd;
  if (!list_empty(&thread_current()->fd_list))
  {
    new_fd = (list_entry(list_back(&thread_current()->fd_list), struct file_descriptor, elem)->fd) + 1;
  }
  else
  {
    new_fd = 3;
  }
  if(!strcmp(thread_current()->name,file))
    file_deny_write(open_file);

  struct list_elem *e = (struct list_elem *)malloc(sizeof(struct list_elem));
  e->next = NULL;
  e->prev = NULL;
  struct file_descriptor *fd = (struct file_descriptor *)malloc(sizeof(struct file_descriptor));
  fd->fd = new_fd;
  fd->file = open_file;
  fd->dir = NULL;
  if(file_get_inode(open_file) != NULL && file_is_dir(open_file)){
    fd->dir = dir_open(inode_reopen(file_get_inode(open_file)));
  }
  memcpy(&(fd->elem),e,sizeof(struct list_elem));
  free(e);
  
  list_push_back(&thread_current()->fd_list, &fd->elem);
  lock_release(&sys_lock);
  return new_fd;
}

int filesize(int fd)
{
  lock_acquire(&sys_lock);
  struct file_descriptor *file = fd_to_fd(fd);
  if (file == NULL)
  {
    lock_release(&sys_lock);
    return -1;
  }
  int result = file_length(file->file);
  lock_release(&sys_lock);
  return result;
}

int read(int fd, void *buffer, unsigned size)
{
  lock_acquire(&sys_lock);
  is_valid_arg(buffer);
  // printf("buffer : %p\n",pg_round_down(buffer));
  int result;
  if (fd == 0)
  {
    result = input_getc();
    lock_release(&sys_lock);
    return result;
  }
  struct file_descriptor *file = fd_to_fd(fd);
  if (file == NULL){
    lock_release(&sys_lock);
    return -1;
  }

  set_evict_file(buffer,size,true);
  int count = 0;

  result = file_read(file->file, buffer, size);
  set_evict_file(buffer,size,false);
  
  lock_release(&sys_lock);
  return result;
}

int write(int fd, const void *buffer, unsigned size)
{
  lock_acquire(&sys_lock);

  is_valid_arg(buffer);
  int result;
  if (fd == 1)
  {
    putbuf(buffer, size); //??????????????
    lock_release(&sys_lock);
    return size;
  }
  else
  {
    struct file_descriptor *file = fd_to_fd(fd);
    if (file == NULL)
    {
      lock_release(&sys_lock);
      return -1;
    }
    if(file_is_dir(file->file)){
      lock_release(&sys_lock);
      return -1;
    }
    set_evict_file(buffer,size,true);
    result = file_write(file->file, buffer, size);
    set_evict_file(buffer,size,false);


    lock_release(&sys_lock);
    return result;
  }
}

void seek(int fd, unsigned position)
{
  lock_acquire(&sys_lock);
  struct file * f = fd_to_fd(fd)->file;
  if(!f){
    lock_release(&sys_lock);
    return;
  }
  file_seek(f, position);
  lock_release(&sys_lock);
}

unsigned tell(int fd)
{
  lock_acquire(&sys_lock);
  unsigned result = file_tell(fd_to_fd(fd)->file);
  lock_release(&sys_lock);
  return result;
}

void close(int fd)
{
  // ?????? fd??? free?????????
  lock_acquire(&sys_lock);
  struct file_descriptor *temp = fd_to_fd(fd);
  if (temp != NULL)
  {
    list_remove(&(temp->elem));
    file_close(temp->file);
    if(temp->dir != NULL){
      dir_close(temp->dir);
    }
    free(temp);
    lock_release(&sys_lock);
  }
  else
  {
    lock_release(&sys_lock);
    exit(-1);
  }
}

int mmap(int fd, void *addr){
  is_valid_arg(addr);
  if(fd == 0 || fd == 1){
    return -1;
  }
  if(addr == 0 || addr == NULL){
    return -1;
  }
  if(pg_ofs(addr) != 0){
    return -1;
  }
  int mapid = fd;
  

  struct file_descriptor *file = fd_to_fd(fd);
  struct file* f;
  
  if(!file)
    return -1;

  lock_acquire(&sys_lock);
  f = file_reopen(file->file);
  // f = file->file;
  off_t offset = 0;
  void * file_end = addr + file_length(file->file);
  if(file_end - addr == 0){
    lock_release(&sys_lock);
    return -1;
  }
  for(void *p = addr; p < file_end; p += PGSIZE){
    if(spt_get_spte(p)){  // ?????? p??? upage??? ?????? spte??? ????????? return -1
      lock_release(&sys_lock);
      return -1;
    }
    uint32_t read_bytes = PGSIZE;
    if(p+PGSIZE > file_end){ 
      read_bytes = file_end - p;
    }
    uint32_t zero_bytes = PGSIZE-read_bytes;
    struct spte* spte = spte_init(p,VM_EXEC_FILE,f, p - addr ,read_bytes,zero_bytes,true); 
    mmape_init(mapid,p,addr,f,spte);
  } 
  // printf("mapid : %d\n",mapid);
  lock_release(&sys_lock);
  return mapid;
}


void munmap(int id){
  struct list_elem * temp;
  struct list * mmape_list = &thread_current()->mmap_list;
  struct thread * cur = thread_current();
  struct fte * fte;
  void * kpage;
  if(list_empty(mmape_list))
    return;
  lock_acquire(&sys_lock);
  for (temp = list_front(mmape_list); temp->next!= NULL ; temp = list_next(temp)){ //munmap ??????
    struct mmape* mmape = list_entry(temp,struct mmape,elem); 
    if(mmape-> mapid != id){
      continue;
    }
    if(mmape->spte->status == VM_SWAP_DISK || mmape->spte->status == VM_EXEC_FILE){
      fte = frame_alloc(mmape->spte,PAL_USER);
    }

    if(mmape->spte->status == VM_ON_MEMORY){
      fte = spte_to_fte(mmape->spte);
      fte->inevictable = true;
      if(pagedir_is_dirty(thread_current()->pagedir, mmape->spte->upage)){
        uint32_t offset = mmape->addr - mmape->file_addr;
        uint32_t size = PGSIZE;
        if(offset + PGSIZE > file_length(mmape->file)){
          size = file_length(mmape->file) - offset;
        }
        // printf("exit unmap %s\n",thread_current()->name);
        file_write_at(mmape->file, mmape-> spte->upage, size , offset);
      }

    pagedir_clear_page(cur->pagedir,mmape->addr);
    }
    
    hash_delete(&cur->spt,&mmape->spte->elem);
    fte->inevictable = false;
    list_remove(&fte->elem);
    free(mmape->spte);
    list_remove(&mmape->elem);
  }
  lock_release(&sys_lock);
}

bool chdir(const char *dir){
  lock_acquire(&sys_lock);
  struct dir* new_dir = open_directories(dir);
  bool success = new_dir!=NULL;
  if(success){
    dir_close(thread_current()->dir);
    thread_current()->dir = new_dir;
    // printf("%p\n",new_dir);
  }
  lock_release(&sys_lock);
  return success;
}

bool mkdir(const char *dir){
  lock_acquire(&sys_lock);
  bool result = filesys_create(dir,0,1);
  lock_release(&sys_lock);
  return result;
}

bool readdir(int fd, char* name){
  lock_acquire(&sys_lock);
  struct file_descriptor * file = fd_to_fd(fd);
  if(file == NULL){
    lock_release(&sys_lock);  
    return false;
  }
  bool result = false;
  if(!file_is_dir(file->file)){
    lock_release(&sys_lock);  
    return false;
  }
  result = dir_readdir(file->dir,name);
  // printf("%s %d\n",name,result);
  lock_release(&sys_lock);  
  return result;
}

bool isdir(int fd){
  lock_acquire(&sys_lock);
  struct file* file = fd_to_fd(fd)->file;
  lock_release(&sys_lock);
  return file_is_dir(file);
}

int inumber(int fd){
  lock_acquire(&sys_lock);
  struct file_descriptor * file = fd_to_fd(fd);
  if(file == NULL){
      lock_release(&sys_lock);
      return -1;
  }
  lock_release(&sys_lock);
  return inode_to_inum(file->file);

}


void set_evict_file(void *buffer, unsigned size, bool inevictable){
    unsigned bound = buffer + size;
    for(void *temp = pg_round_down(buffer); temp<bound; temp+=PGSIZE){
        if(get_user(temp)== -1){
            exit(-1);
        } 
        struct spte * spte = spt_get_spte(temp);
        if(!spte){
            // printf("no spte\n");
            exit(-1);
        }
        struct fte* fte = spte_to_fte(spte);
        fte->inevictable = inevictable;
    }
}

