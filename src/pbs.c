/*****************************************************************************
 * pbs.c: Print Boot Sector
 *
 * Authors: Joey Gallahan & David Jordan
 * 
 * Description: Performs the pbs command, which prints the contents of the
 *              boot sector.
 *
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 ****************************************************************************/

#include "fat.h"
#include <stdio.h>
#include <string.h>


int main()
{
  if (initializeFatFileSystem() != 0)
    return -1;
  
  // Retreive the boot sector information.
  FatBootSector bootSector;    
  getFatBootSector(&bootSector);

  // Copy the volume-label into a null-terminated string.
  char volumeLabel[12];
  memcpy(volumeLabel, bootSector.volumeLabel, 11);
  volumeLabel[11] = '\0';

  // Copy the file-system-type into a null-terminated string.
  char fileSystemType[9];
  memcpy(fileSystemType, bootSector.fileSystemType, 8);
  fileSystemType[8] = '\0';

  // Print out the boot sector information.
  printf("Bytes Per Sector           = %d\n", bootSector.bytesPerSector);
  printf("Sectors Per Cluster        = %d\n", bootSector.sectorsPerCluster);
  printf("Number of FATs             = %d\n", bootSector.numFATs);
  printf("Number of Reserved Sectors = %d\n", bootSector.numReservedSectors);
  printf("Number of Root Entries     = %d\n", bootSector.maxNumRootDirEntries);
  printf("Total Sector Count         = %d\n", bootSector.totalSectorCount);
  printf("Sectors Per Fat            = %d\n", bootSector.sectorsPerFAT);
  printf("Sectors Per Track          = %d\n", bootSector.sectorsPerTrack);
  printf("Number of Heads            = %d\n", bootSector.numHeads);
  printf("Boot Signature (in hex)    = 0x%02X\n", bootSector.bootSignature);
  printf("Volume ID (in hex)         = 0x%06X\n", bootSector.volumeID);
  printf("Volume Label               = %s\n", volumeLabel);
  printf("File System Type           = %s\n", fileSystemType);

  terminateFatFileSystem();
  return 0;
}


