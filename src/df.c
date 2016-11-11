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
  
  unsigned int totalBlocks;
  unsigned int numUsedBlocks;
  
  getNumberOfUsedBlocks(&numUsedBlocks, &totalBlocks);
  
  unsigned int numAvailableBlocks = totalBlocks - numUsedBlocks;
  float usePercent = ((float) numUsedBlocks / (float) totalBlocks) * 100.0f;
  
  printf("%15s%10s%15s%11s\n", "512K-blocks", "Used", "Available", "Use %");
  printf("%15u%10u%15u%11.2f\n", totalBlocks, numUsedBlocks,
         numAvailableBlocks, usePercent);  
  
  int i;
  /*
  long int numBytes = strtol(argv[1], NULL, 10);
  unsigned char* data = (unsigned char*) malloc(numBytes);
  for (i = 0; i < numBytes; i++)
    data[i] = '0' + (i % 10);
  writeFileContents(2, data, (unsigned int) numBytes);
  free(data);
  
  
  unsigned char* readData;
  unsigned int readNumBytes;
  
  readFileContents(2, &readData, &readNumBytes);
  printf("readNumBytes = %u bytes (%u sectors)\n", readNumBytes, readNumBytes / 512);
  
  for (i = 0; i < readNumBytes; i++)
    printf("%c", readData[i]);
  printf("\n");

  free(readData);
  */

  terminateFatFileSystem();
  return 0;
}

