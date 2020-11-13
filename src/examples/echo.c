#include <stdio.h>
#include <syscall.h>


int
main (int argc, char **argv)
{
  // int i;
  // for (i = 0; i < argc; i++)
  //   printf ("%s ", argv[i]);
  // printf ("\n"); 
  printf("create start : \n");
  create("a",10);
  create("b",10);
  create("c",10);
  printf("open start : \n");
  open("a");
  open("b");
  open("b");
  // open()
  return EXIT_SUCCESS;
}
