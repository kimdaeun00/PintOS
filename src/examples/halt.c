/* halt.c

   Simple program to test whether running a user program works.
 	
   Just invokes a system call that shuts down the OS. */

#include <syscall.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  // halt();
  printf("hi there!\n");
  exit(32);
  /* not reached */
}
