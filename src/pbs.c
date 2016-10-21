#include "fat.h"
#include <stdio.h>
#include <string.h>

void printBootSector();

int main()
{

   // Open the disk image file.
   FILE_SYSTEM_ID = fopen("../disks/floppy1", "r+");
   if (FILE_SYSTEM_ID == NULL)
   {
      printf("Could not open the floppy drive or image.\n");
      return 1;
   }

   // Load the boot sector.
   if (loadFAT12BootSector() != 0)
   {
      printf("Something has gone wrong -- could not read the boot table\n");
      return 1;
   }
  // stuff = (unsigned char*) malloc(BYTES_PER_SECTOR);
   printBootSector();

   return 0;
}

void printBootSector()
{   
   char volumeLabel[12];
   memcpy(volumeLabel, FAT_BOOT_SECTOR.volumeLabel, 11);
   volumeLabel[11] = '\0';
   
   char fileSystemType[9];
   memcpy(fileSystemType, FAT_BOOT_SECTOR.fileSystemType, 8);
   fileSystemType[8] = '\0';
   
   printf("Bytes Per Sector: %d\n", FAT_BOOT_SECTOR.bytesPerSector);
   printf("Sectors Per Cluster: %d\n", FAT_BOOT_SECTOR.sectorsPerCluster);
   printf("Number of FATs: %d\n", FAT_BOOT_SECTOR.numFATs);
   printf("Number of Reserved Sectors: %d\n", FAT_BOOT_SECTOR.numReservedSectors);
   printf("Number of Root Entries: %d\n", FAT_BOOT_SECTOR.maxNumRootDirEntries);
   printf("Total Sector Count: %d\n", FAT_BOOT_SECTOR.totalSectorCount);
   printf("Sectors Per Fat: %d\n", FAT_BOOT_SECTOR.sectorsPerFAT);
   printf("Sectors Per Track: %d\n", FAT_BOOT_SECTOR.sectorsPerTrack);
   printf("Number of Heads: %d\n", FAT_BOOT_SECTOR.numHeads);
   printf("Boot Signature (hex): 0x%02X\n", FAT_BOOT_SECTOR.bootSignature);
   printf("Volume ID (hex): 0x%06X\n", FAT_BOOT_SECTOR.volumeID);
   printf("Volume Label: %s\n", volumeLabel);
   printf("File System Type: %s\n", fileSystemType);
}


