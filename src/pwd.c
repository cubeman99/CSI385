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
 * usage - prints the usage statement.
 *****************************************************************************/
static void usage()
{
	printf("Usage: pfe X Y\n");
	printf("Prints the 12-bit FAT entry values representing logical sectors X to Y.\n");
}

void printDir(unsigned int sector, int depth)
{
  unsigned char bytes[BYTES_PER_SECTOR];
  unsigned int ss = sector;
  if (sector == 0)
    ss = 1 + fatFileSystem.bootSector.sectorsPerFAT * 2;
  else
    ss = 33 + sector - 2;
  read_sector(ss, bytes);

  DirectoryEntry* dir = (DirectoryEntry*) bytes;

  int i, k;
  for (k = 0; k < 20; k++)	
  {
    if (depth >= 16)
    {
      for (i = 0; i < depth; i++)
        printf("   ");
      printf(" (too deep)\n");
      return;
    }

    if ((unsigned char) dir[k].name[0] == 0xE5)
    {
      // The directory entry is free (i.e., currently unused).
    }
    else if ((unsigned char) dir[k].name[0] == 0x00)
    {
      
      // This directory entry is free and all the remaining directory
      // entries in this directory are also free.
      break;
    }
    else if (dir[k].attributes == 0x0F)
    {
      // Long filename
    }
    else
    {   
      for (i = 0; i < depth; i++)
        printf("   ");


      int isSubDir = (dir[k].attributes & 0x10);

      if (isSubDir)
        printf("* ");
      else
        printf("- ");
      //printf("%d: ", k);  

      for (i = 0; i < 8; i++)
      {
        if (dir[k].name[i] == ' ')
          break;
        printf("%c", dir[k].name[i]);
      }
      if (dir[k].extension[0] != ' ')
      {
        printf(".");
        for (i = 0; i < 3; i++)
        {
          if (dir[k].extension[i] == ' ')
            break;
          printf("%c", dir[k].extension[i]);
        }
      }

      if (dir[k].attributes & 0x01)
        printf(" (read-only)");
      if (dir[k].attributes & 0x02)
        printf(" (hidden)");
      if (dir[k].attributes & 0x04)
        printf(" (system)");

      if (isSubDir)
      {
        printf(" (%u)\n", dir[k].firstLogicalCluster);
        if (k >= 2 || sector == 0)
          printDir(dir[k].firstLogicalCluster, depth + 1);
      }
      else
      {
        printf("\n");
      }
    }
   } 
}


/******************************************************************************
 * main - runs the pfe command.
 *****************************************************************************/
int main(int argc, char* argv[])
{
  initializeFatFileSystem();

  printf("sz = %u\n", sizeof(DirectoryEntry));

  printDir(0, 0);

  terminateFatFileSystem();
  return 0;
}

