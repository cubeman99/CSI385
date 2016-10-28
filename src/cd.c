/******************************************************************************
 * cd.c: Change directory
 *
 * Author: David Jordan
 *
 * Description: Performs the cd command, which changes the current working
 *              directory using the given path name. If no path is given, then
 *              it changes to the root directory.
 *
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 *****************************************************************************/

#include <stdio.h>
#include "fat.h"


int main(int argc, char* argv[])
{
  if (initializeFatFileSystem() != 0)
    return -1;
  
  if (argc == 1)
  {
    changeWorkingDirectory("/");
  }
  else if (argc == 2)
  {
    changeWorkingDirectory(argv[1]);
  }
  else
  {
    printf("Error: too many arguments for cd command\n");
    printf("usage: cd [PATH]\n");
  }
  
  terminateFatFileSystem();
  return 0;
}

