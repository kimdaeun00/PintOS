#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "threads/pte.h"

struct lock sys_lock;
struct lock *syscall_lock = &sys_lock;

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
    if (!is_user_vaddr((void *)(temp)) || !is_user_vaddr((void *)(((char *)(temp)) + 3)))
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
  // if(*esp != 6)
  //   printf("syscall num : %d\n",*esp);
  // printf("content in esp : %d\n",*(char *)(((char *)(esp))));
  // printf("address in esp : %p\n",(char *)(((char *)(esp))));
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
  result = filesys_create(file, initial_size);
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
  memcpy(&(fd->elem),e,sizeof(struct list_elem));
  free(e);
  
  list_push_back(&thread_current()->fd_list, &fd->elem);
  lock_release(&sys_lock);

  return new_fd;
}

int filesize(int fd)
{
  struct file_descriptor *file = fd_to_fd(fd);
  if (file == NULL)
  {
    return -1;
  }
  lock_acquire(&sys_lock);
  int result = file_length(file->file);
  lock_release(&sys_lock);
  return result;
}

int read(int fd, void *buffer, unsigned size)
{
  is_valid_arg(buffer);
  int result;
  lock_acquire(&sys_lock);
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

  result = file_read(file->file, buffer, size);
  // lock_release(&sys_lock);
  // printf("read : %p\n",buffer);
  // printf("read : %d\n",result);
  // printf("%d\n",strlen(buffer));
  return result;
}

int write(int fd, const void *buffer, unsigned size)
{
  is_valid_arg(buffer);
  int result;
  lock_acquire(&sys_lock);
  if (fd == 1)
  {
    putbuf(buffer, size); //실제로는??
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
    result = file_write(file->file, buffer, size);
    lock_release(&sys_lock);
    return result;
  }
}

void seek(int fd, unsigned position)
{
  lock_acquire(&sys_lock);
  struct file * f = fd_to_fd(fd)->file;
  if(f == NULL)
    return;
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
  // 닫힌 fd값 free해주기
  struct file_descriptor *temp = fd_to_fd(fd);
  lock_acquire(&sys_lock);
  if (temp != NULL)
  {
    list_remove(&(temp->elem));
    file_close(temp->file);
    free(temp);
    lock_release(&sys_lock);
  }
  else
  {
    lock_release(&sys_lock);
    exit(-1);
  }
}
