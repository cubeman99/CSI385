/******************************************************************************
 * df.c: Disk free
 *
 * Author: David Jordan
 *
 * Description: Performs the df command, which prints the number of free
 *              logical blocks. 
 * 
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 *****************************************************************************/

#include <stdio.h>
#include "fat.h"
#include <stdlib.h>


int main(int argc, char* argv[])
{
  if (initializeFatFileSystem() != 0)
    return -1;
  
  unsigned short totalBlocks;
  unsigned short numUsedBlocks;
  
  getNumberOfUsedBlocks(&numUsedBlocks, &totalBlocks);
  
  unsigned short numAvailableBlocks = totalBlocks - numUsedBlocks;
  float usePercent = ((float) numUsedBlocks / (float) totalBlocks) * 100.0f;
  
  printf("%15s%10s%15s%11s\n", "512K-blocks", "Used", "Available", "Use %");
  printf("%15u%10u%15u%11.2f\n", totalBlocks, numUsedBlocks,
         numAvailableBlocks, usePercent);  
  
  terminateFatFileSystem();
  return 0;
}

