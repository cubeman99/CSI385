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

/*
0 11 Ignore
11 2 Bytes per sector
13 1 Sectors per cluster
14 2 Number of reserved sectors
16 1 Number of FATs
17 2 Maximum number of root directory entries
19 2 Total sector count1
21 1 Ignore
22 2 Sectors per FAT
24 2 Sectors per track
26 2 Number of heads
28 4 Ignore
32 4 Total sector count for FAT32 (0 for FAT12 and FAT16)
36 2 Ignore
38 1 Boot signature2
39 4 Volume ID3
43 11 Volume label
54 8 File system type5 (e.g. FAT12, FAT16)
62 - Rest of boot sector (ignore)
*/

#pragma pack(1)
typedef struct
{
  char            ignore1[11];

  unsigned short  bytesPerSector;
  unsigned char   sectorsPerCluster;
  unsigned short  numReservedSectors;
  unsigned char   numFATs;
	unsigned short  maxNumRootDirEntries;
	unsigned short  totalSectorCount;

	char            ignore2[1];

	unsigned short  sectorsPerFAT;
	unsigned short  sectorsPerTrack;
	unsigned short  numHeads;

	char            ignore3[4];

	unsigned int    totalSectorCountForFAT32;

	char            ignore4[2];

	unsigned char   bootSignature;
	unsigned int    volumeID;
	char            volumeLabel[11];
	char            fileSystemType[8];

	// Rest of boot sector is ignored.

} BootSector;
#pragma pack()


BootSector FAT_BOOT_SECTOR;


void usage()
{
	printf("Usage: pfe X Y\n");
	printf("Prints the 12-bit FAT entry values representing logical sectors x to y.\n");
}

int loadFAT12BootSector()
{
  // Set it to this only to read the boot sector, then reset it per the
  // value in the boot sector.
  BYTES_PER_SECTOR = sizeof(BootSector);

  // Read the part of the boot sector we care about.
  if (read_sector(0, (unsigned char*) &FAT_BOOT_SECTOR) == -1)
	{
		return -1;
	}

  // Now change this variable to the actual bytes-per-sector.
  BYTES_PER_SECTOR = FAT_BOOT_SECTOR.bytesPerSector;
	
	return 0;
}


unsigned char* readFAT12Table(int fatIndex)
{
	// Allocate the FAT buffer.
	unsigned char* buffer = (unsigned char*) malloc(BYTES_PER_SECTOR *
		FAT_BOOT_SECTOR.sectorsPerFAT);

	// Read the FAT table from the file.
  if (read_sector(1 + fatIndex, buffer) == -1)
	{
		free(buffer);
		return NULL;
	}

	return buffer;
}

void freeFAT12Table(unsigned char* fatTable)
{
	free(fatTable);
}

int main(int argc, char* argv[])
{
	// Validate the number of arguments.
	if (argc != 3)
	{
		printf("Invalid number of arguments, expected 2.\n");
		usage();
		return 1;
	}

	// Parse the first argument X.
	char* endptr;
	int x = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0')
	{
		printf("X must be a number\n");
		usage();
		return 1;
	}

	// Parse the second argument Y.
	int y = strtol(argv[2], &endptr, 10);
	if (*endptr != '\0')
	{	
		printf("Y must be a number\n");
		usage();
		return 1;
	}
	
	// Validate the two arguments.
	if (x < 2)
	{
		printf("X must be 2 or larger\n");
		usage();
		return 1;
	}
	else if (y < x)
	{
		printf("Y cannot be less than X\n");
		usage();
		return 1;
	}

  BootSector& bootSector = FAT_BOOT_SECTOR;

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

	// Read the first FAT table.
	unsigned char* fat = readFAT12Table(0);
  if (fat == NULL)
	{
    printf("Something has gone wrong -- could not read the FAT table\n");
		return 1;
	}
	
	// Print out the FAT entries.
	for (int i = x; i < y; i++)
	{
		int entry = get_fat_entry((unsigned int) i + 1, fat);
		printf("Entry %d: %X\n", i + 1, entry);
	}

	freeFAT12Table(fat);
  return 0;
}
