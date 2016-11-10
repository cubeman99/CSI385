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


int main(int argc, char* argv[])
{
  if (initializeFatFileSystem() != 0)
    return -1;
  
  int totalBlocks;
  int numUsedBlocks;
  
  getNumberOfUsedBlocks(&numUsedBlocks, &totalBlocks);
  
  int numAvailableBlocks = totalBlocks - numUsedBlocks;
  float usePercent = ((float) numUsedBlocks / (float) totalBlocks) * 100.0f;
  
  printf("%15s%10s%15s%11s\n", "512K-blocks", "Used", "Available", "Use %");
  printf("%15d%10d%15d%11.2f\n", totalBlocks, numUsedBlocks,
         numAvailableBlocks, usePercent);  

  terminateFatFileSystem();
  return 0;
}

