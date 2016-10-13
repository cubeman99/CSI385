#ifndef _FAT_H_
#define _FAT_H_

#include "fatSupport.h"
#include <stdio.h>

/*
 0 11 Ignore
11  2 Bytes per sector
13  1 Sectors per cluster
14  2 Number of reserved sectors
16  1 Number of FATs
17  2 Maximum number of root directory entries
19  2 Total sector count1
21  1 Ignore
22  2 Sectors per FAT
24  2 Sectors per track
26  2 Number of heads
28  4 Ignore
32  4 Total sector count for FAT32 (0 for FAT12 and FAT16)
36  2 Ignore
38  1 Boot signature2
39  4 Volume ID3
43 11 Volume label
54  8 File system type5 (e.g. FAT12, FAT16)
62 -- Rest of boot sector (ignore)
*/

/******************************************************************************
 * BootSector - struct containing the data elements in the FAT boot sector.
 *****************************************************************************/
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

/******************************************************************************
 * loadFAT12BootSector
 *****************************************************************************/
int loadFAT12BootSector();

/******************************************************************************
 * readFAT12Table
 *****************************************************************************/
unsigned char* readFAT12Table(int fatIndex);

/******************************************************************************
 * freeFAT12Table
 *****************************************************************************/
void freeFAT12Table(unsigned char* fatTable);


// Some global variables used by FAT functions.
extern BootSector FAT_BOOT_SECTOR;
extern FILE* FILE_SYSTEM_ID;
extern int BYTES_PER_SECTOR;


#endif //_FAT_H_

