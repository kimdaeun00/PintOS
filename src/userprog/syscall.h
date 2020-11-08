#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>
#include <string.h>
#include "../threads/thread.h"
#include "../filesys/filesys.h"
// #include "../filesys/file.c"
#include "../devices/shutdown.h"
#include "process.h"
#include "../devices/input.h"

// extern struct lock *syscall_lock;

void syscall_init (void);
void halt(void);
void exit(int);
tid_t exec(const char*);
int wait(tid_t);
bool create(const char*, unsigned);
bool remove(const char* );
int open(const char*);
int filesize(int);
int read(int, void *, unsigned );
int write(int,const void*, unsigned);
void seek(int,unsigned);
unsigned tell(int);
void close(int);

#endif /* userprog/syscall.h */
