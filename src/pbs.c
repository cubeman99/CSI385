#include "fat.h"
#include <stdio.h>
#include <string.h>

void printBootSector();

int main()
{
  initializeFatFileSystem();
  
  // Retreive the boot sector information.
  FatBootSector bootSector;    
  getFatBootSector(&bootSector);

  char volumeLabel[12];
  memcpy(volumeLabel, bootSector.volumeLabel, 11);
  volumeLabel[11] = '\0';

  char fileSystemType[9];
  memcpy(fileSystemType, bootSector.fileSystemType, 8);
  fileSystemType[8] = '\0';

  printf("Bytes Per Sector: %d\n", bootSector.bytesPerSector);
  printf("Sectors Per Cluster: %d\n", bootSector.sectorsPerCluster);
  printf("Number of FATs: %d\n", bootSector.numFATs);
  printf("Number of Reserved Sectors: %d\n", bootSector.numReservedSectors);
  printf("Number of Root Entries: %d\n", bootSector.maxNumRootDirEntries);
  printf("Total Sector Count: %d\n", bootSector.totalSectorCount);
  printf("Sectors Per Fat: %d\n", bootSector.sectorsPerFAT);
  printf("Sectors Per Track: %d\n", bootSector.sectorsPerTrack);
  printf("Number of Heads: %d\n", bootSector.numHeads);
  printf("Boot Signature (hex): 0x%02X\n", bootSector.bootSignature);
  printf("Volume ID (hex): 0x%06X\n", bootSector.volumeID);
  printf("Volume Label: %s\n", volumeLabel);
  printf("File System Type: %s\n", fileSystemType);

  terminateFatFileSystem();
  return 0;
}

