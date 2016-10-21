/*****************************************************************************
 * Author: David Jordan & Joey Gallahan
 * 
 * Description: TODO
 *
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "fat.h"

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
FatBootSector FAT_BOOT_SECTOR;

FatFileSystem fatFileSystem;


/******************************************************************************
 * loadFAT12BootSector
 *****************************************************************************/
int loadFAT12BootSector()
{
/*
  // Set it to this only to read the boot sector, then reset it per the
  // value in the boot sector.
  BYTES_PER_SECTOR = sizeof(FatBootSector);

  // Read the part of the boot sector we care about.
  if (read_sector(0, (unsigned char*) &FAT_BOOT_SECTOR) == -1)
	{
		return -1;
	}

  // Now change this variable to the actual bytes-per-sector.
  BYTES_PER_SECTOR = FAT_BOOT_SECTOR.bytesPerSector;
	*/

  // Set it to this only to read the boot sector, then reset it per the
  // value in the boot sector.
  BYTES_PER_SECTOR = sizeof(FatBootSector);

  // Read the part of the boot sector we care about.
  if (read_sector(0, (unsigned char*) &fatFileSystem.bootSector) == -1)
	{
		return -1;
	}

  // Now change this variable to the actual bytes-per-sector.
  BYTES_PER_SECTOR = fatFileSystem.bootSector.bytesPerSector; 

  FAT_BOOT_SECTOR = fatFileSystem.bootSector;

	return 0;
}

/******************************************************************************
 * readFAT12Table
 *****************************************************************************/
unsigned char* readFAT12Table(int fatIndex)
{
	// Allocate the FAT buffer.
	unsigned char* buffer = (unsigned char*) malloc(BYTES_PER_SECTOR *
		fatFileSystem.bootSector.sectorsPerFAT);

	// Read the FAT table from the file.
  if (read_sector(1 + fatIndex, buffer) == -1)
	{
		free(buffer);
		return NULL;
	}

	return buffer;
}

/******************************************************************************
 * freeFAT12Table
 *****************************************************************************/
void freeFAT12Table(unsigned char* fatTable)
{
	free(fatTable);
}









int initializeFatFileSystem()
{
  // Open the disk image file.
  FILE_SYSTEM_ID = fopen("../disks/floppy2", "r+");
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
	fatFileSystem.fatTable = readFAT12Table(0);
  if (fatFileSystem.fatTable == NULL)
	{
    printf("Something has gone wrong -- could not read the FAT table\n");
		return 1;
	}

  loadWorkingDirectory();



  return 0;
}

int loadWorkingDirectory()
{
  
  if (access(WORKING_DIRECTORY_FILE_NAME, F_OK) != -1)
  {
    // file exists
    printf("CWD exists\n");

    FILE* cwdFile = fopen(WORKING_DIRECTORY_FILE_NAME, "r");

    if (cwdFile == NULL)
	  {
      printf("Could not initialze current working directory.\n");
      return 1;
    }

    // Read the path name from the file.
    fgets(fatFileSystem.workingDirectory.pathName,
      sizeof(fatFileSystem.workingDirectory.pathName), cwdFile);

    // Remove the newline character from the end of the path name.
    char *pos = strchr(fatFileSystem.workingDirectory.pathName, '\n');
    if (pos != NULL)
        *pos = '\0';

    // TODO: sConstruct the directory levels from the path.
    

    printf("pwd = \"%s\"\n", fatFileSystem.workingDirectory.pathName);

    fclose(cwdFile);
  }
  else
  {
    // file doesn't exist
    printf("CWD does NOT exist\n");
    
    // default to root directory.
    strcpy(fatFileSystem.workingDirectory.pathName, "/");
    fatFileSystem.workingDirectory.depthLevel = 1;
    fatFileSystem.workingDirectory.directoryLevels[0].indexInParentDirectory = 0;
    fatFileSystem.workingDirectory.directoryLevels[0].firstLogicalCluster = 0;

    saveWorkingDirectory();
  }

  return 0;
}

int saveWorkingDirectory()
{
  FILE* cwdFile = fopen(WORKING_DIRECTORY_FILE_NAME, "w");

  if (cwdFile == NULL)
  {
    printf("Could not initialze current working directory.\n");
    return 1;
  }

  fprintf(cwdFile, "%s\n", fatFileSystem.workingDirectory.pathName);

  fclose(cwdFile);
  return 0;
}

int terminateFatFileSystem()
{
	free(fatFileSystem.fatTable);
  fclose(FILE_SYSTEM_ID);
  return 0;
}

int getFatBootSector(FatBootSector* bootSector)
{
  return 0;
}








