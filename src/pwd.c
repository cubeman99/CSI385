/******************************************************************************
 * pwd.c: Print working directory.
 *
 * Author: David Jordan & Joey Gallahan
 *
 * Description: Performs the pwd command, which prints the absolute path name
 *              of the current working directory.
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
    
  FilePath workingDir;
  getWorkingDirectory(&workingDir);
  
  printf("%s\n", workingDir.pathName); 

  terminateFatFileSystem();
  return 0;
}

