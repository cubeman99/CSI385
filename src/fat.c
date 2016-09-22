/******************************************************************************
 * main: Sample for starting the FAT project.
 *
 * Authors:  Andy Kinley, Archana Chidanandan, David Mutchler and others.
 *           March, 2004.
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "fatSupport.h"

// 13 is NOT the correct number -- you fix it!
#define BYTES_TO_READ_IN_BOOT_SECTOR 64

/******************************************************************************
 * You must set these global variables:
 *    FILE_SYSTEM_ID -- the file id for the file system (here, the floppy disk
 *                      filesystem)
 *    BYTES_PER_SECTOR -- the number of bytes in each sector of the filesystem
 *
 * You may use these support functions (defined in fatSupport.c)
 *    read_sector
 *    write_sector
 *    get_fat_entry
 *    set_fat_entry
 *****************************************************************************/

FILE* FILE_SYSTEM_ID;
int BYTES_PER_SECTOR;


/******************************************************************************
 * main: an example of reading an item in the boot sector
 *****************************************************************************/

#pragma pack(1)
typedef struct
{
  char            ignore1[11];
  unsigned short  bytesPerSector;
  unsigned char   sectorsPerCluster;
  unsigned short  numReservedSectors;

  // TODO: Add the reset of the boot sector here...

} BootSector;
#pragma pack()

int main()
{
  BootSector bootSector;

  // Open the disk image file.
  FILE_SYSTEM_ID = fopen("../disks/floppy1", "r+");
  if (FILE_SYSTEM_ID == NULL)
  {
    printf("Could not open the floppy drive or image.\n");
    return 1;
  }

  // Set it to this only to read the boot sector, then reset it per the
  // value in the boot sector.
  BYTES_PER_SECTOR = sizeof(BootSector);

  // Read the part of the boot sector we care about.
  if (read_sector(0, (unsigned char*) &bootSector) == -1)
    printf("Something has gone wrong -- could not read the boot sector\n");

  // Now change this variable to the actual bytes-per-sector.
  BYTES_PER_SECTOR = bootSector.bytesPerSector;

  // Print out some info about the boot sector.
  printf("bytes per sector     = %d\n", bootSector.bytesPerSector);
  printf("sectors per cluster  = %d\n", bootSector.sectorsPerCluster);
  printf("num reserved sectors = %d\n", bootSector.numReservedSectors);

  return 0;
}
