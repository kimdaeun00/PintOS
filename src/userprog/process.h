#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
char **get_argv(char*);
int get_argc(char*);
void stack_put(char**,int,void**);
int calculate_word_align(int, char**);

#endif /* userprog/process.h */
