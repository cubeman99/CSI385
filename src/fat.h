#ifndef _FAT_H_
#define _FAT_H_

#include "fatSupport.h"
#include <stdio.h>


#define FAT12_MAX_FILE_NAME_LENGTH 12
#define FAT12_MAX_DIRECTORY_DEPTH 32
#define FAT12_MAX_PATH_NAME_LENGTH (12 * 32)

#define WORKING_DIRECTORY_FILE_NAME "./cwd.txt"

typedef unsigned char* FatTable;
typedef char* Path;


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

} FatBootSector;

typedef struct
{
	char           name[8];
	char           extension[3];
	unsigned char  attributes;
	unsigned char  reserved[2];
	unsigned short creationTime;
	unsigned short creationDate;
	unsigned short lastAccessDate;
	unsigned char  ignored[2];
	unsigned short lastWriteTime;
	unsigned short lastWriteDate;
	unsigned short firstLogicalCluster;
	unsigned int   fileSize;
} DirectoryEntry;

typedef struct
{
  unsigned int depthLevel; // 1 = root directory
  
  struct
  {
	  unsigned int   indexInParentDirectory; // negligable for root directory
	  unsigned short firstLogicalCluster;
  } directoryLevels[FAT12_MAX_DIRECTORY_DEPTH];

	char pathName[FAT12_MAX_PATH_NAME_LENGTH];  
  
} WorkingDirectory;

typedef struct
{
	FatBootSector    bootSector;
	FatTable         fatTable;
	WorkingDirectory workingDirectory;
} FatFileSystem;

#pragma pack()

/******************************************************************************
 * loadFAT12BootSector
 *****************************************************************************/
int loadFAT12BootSector();

/******************************************************************************
 * readFAT12Table
 *****************************************************************************/
unsigned char* readFAT12Table(int fatIndex);

/************************ ******************************************************
 * freeFAT12Table
 *****************************************************************************/
void freeFAT12Table(unsigned char* fatTable);



int initializeFatFileSystem();
int terminateFatFileSystem();
int getFatBootSector(FatBootSector* bootSector);

int loadWorkingDirectory();
int saveWorkingDirectory();

// Some global variables used by FAT functions.
extern FatBootSector FAT_BOOT_SECTOR;
extern FILE* FILE_SYSTEM_ID;
extern int BYTES_PER_SECTOR;

extern FatFileSystem fatFileSystem;


#endif //_FAT_H_

