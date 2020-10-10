#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/pte.h"


static struct file_descriptor* fd_to_fd(int fd)
{
  struct list *fd_list = &thread_current()->fd_list;
  struct list_elem *e;
  struct file_descriptor* target;

  if(list_size(fd_list)==0)
    return NULL;

  for(e=list_front(fd_list); e!=list_tail(fd_list) ; e = e->next){
    target = list_entry(e,struct file_descriptor, elem);
    if(target->fd == fd){
      return target;
    }
  }
  return NULL;
}

static bool is_userspace(int *esp, int n){
  int * temp;
  for(int i=0;i<n+1;i++){
    temp = esp+i;
    if(!is_user_vaddr((void *)(temp))|| !is_user_vaddr((void *)(((char *)(temp))+3))){
      printf("error 2\n");
      return false;
    }
  }
  return true;
}

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f) 
{
  int *esp = (int*)(f->esp);
  // printf("%p\n",esp);
  // hex_dump(esp,esp,100,1);  
  // printf("%d\n",*(char *)(((char *)(esp))+3));
  if(pagedir_get_page (thread_current()->pagedir, esp)==NULL){
    printf("error 1\n");
    exit(-1);
  }

  switch (*esp)
  {
  case SYS_HALT:
  
    if(!is_userspace(esp,0)){
      exit(-1);
    }
    halt();
    break;
  
  case SYS_EXIT:
    if(!is_userspace(esp,1)){
      exit(-1);
    }
    exit(*(esp+1));
    break;

  case SYS_EXEC:
    // exec((char *)(*(esp+1)));
    break;
  
  case SYS_WAIT:
  
    break;
  
  case SYS_CREATE:
    if(!is_userspace(esp,2)){
      exit(-1);
    }
    create((const char *)(*(esp+1)),*(unsigned *)(esp+2));
    break;
  
  case SYS_REMOVE:
    if(!is_userspace(esp,1)){
      exit(-1);
    }
    remove((const char *)(*(esp+1)));
    break;
  
  case SYS_OPEN:
    if(!is_userspace(esp,1)){
      exit(-1);
    }
    open((const char *)(*(esp+1)));
    break;
  
  case SYS_FILESIZE:
    if(!is_userspace(esp,1)){
      exit(-1);
    }
    filesize(*(int*)(esp+1));
    break;
  
  case SYS_READ:
    if(!is_userspace(esp,3)){
      exit(-1);
    }
    read(*(int*)(esp+1),(void *)(*(esp+2)),*(unsigned *)(esp+3));
    break;
  
  case SYS_WRITE:
    if(!is_userspace(esp,3)){
      exit(-1);
    }
    write(*(int*)(esp+1),(const char *)(*(esp+2)),*(unsigned *)(esp+3));

    break;
  
  case SYS_SEEK:
    if(!is_userspace(esp,1)){
      exit(-1);
    }
    seek(*(int*)(esp+1),*(unsigned int *)(esp+1));
    break;
  
  case SYS_TELL:
    if(!is_userspace(esp,1)){
      exit(-1);
    }
    tell(*(int *)(esp+1));
    break;
  
  case SYS_CLOSE:
    if(!is_userspace(esp,1)){
      exit(-1);
    }
    close(*(int *)(esp+1));
    break;
    
  default:
    break;
  }

}

void
halt(void)
{
  shutdown_power_off();
}


void
exit(int status)
{
  thread_current()->exit_status = status;
  thread_exit();
}

void
exec(const char *cmd_line)
{

}

int
wait(tid_t pid)
{
  return process_wait(pid);
}

bool create(const char* file, unsigned initial_size)
{
  return filesys_create(file,initial_size);
}

bool remove(const char* file)
{
  return filesys_remove(file);
}

int open(const char *file)
{
  struct file* open_file = filesys_open(file);
  struct list_elem e;

  if(open_file == NULL)
    return -1;

  int new_fd = (list_entry(list_back(&thread_current()->fd_list),struct file_descriptor,elem)->fd) + 1;
  struct file_descriptor fd = {new_fd,open_file,e};
  list_push_back(&thread_current()->fd_list,&(fd.elem));
  return new_fd;
}

int filesize(int fd)
{
  struct file_descriptor* file = fd_to_fd(fd);
  if(file == NULL){
    return -1;
  }
  return file_length(file->file);
}


int read(int fd, void *buffer, unsigned size)
{
  struct file_descriptor *file = fd_to_fd(fd);
  if (file == NULL)
    return -1;
  
  if (fd == 0){
    return input_getc();
  }
  else{
    return file_read(file->file,buffer,size);
  }
}

int write(int fd, const void* buffer, unsigned size)
{
  struct file_descriptor *file = fd_to_fd(fd);

  if(fd == 1) //0이면?
  {
    putbuf(buffer,size);
    return size;
  } 
  else
  {
    if(file == NULL){
      return -1;
    }
    return file_write(file->file,buffer,size);
  }
}


void seek(int fd, unsigned position)
{
  return file_seek(fd_to_fd(fd)->file,position);
}

unsigned tell(int fd)
{
  return file_tell(fd_to_fd(fd)->file);
}

void close(int fd)
{
  // 닫힌 fd값 free해주기 
  if(fd_to_fd(fd) != NULL){
    list_remove(&(fd_to_fd(fd)->elem));  
  }
}
