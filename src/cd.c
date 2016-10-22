/******************************************************************************
 * pfe.c: Print FAT entry
 *
 * Author: David Jordan
 *
 * Description: Prints the 12-bit FAT entry values representing logical
 *              sectors X to Y.
 * 
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "fat.h"

/******************************************************************************
 * main - runs the pfe command.
 *****************************************************************************/
int main(int argc, char* argv[])
{
  initializeFatFileSystem();
  
  changeWorkingDirectory(argv[1]);

  terminateFatFileSystem();
  return 0;
}

