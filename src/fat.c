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

  // Calculate some sector offsets.
	fatFileSystem.sectorOffsets.fatTables =
	  fatFileSystem.bootSector.numReservedSectors;
	
  fatFileSystem.sectorOffsets.rootDirectory =
    fatFileSystem.sectorOffsets.fatTables +
    (fatFileSystem.bootSector.sectorsPerFAT *
    fatFileSystem.bootSector.numFATs);
    
	fatFileSystem.sectorOffsets.dataRegion = 
	  fatFileSystem.sectorOffsets.rootDirectory +
	  ((fatFileSystem.bootSector.maxNumRootDirEntries * sizeof(DirectoryEntry)) /
      fatFileSystem.bootSector.bytesPerSector);

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

int terminateFatFileSystem()
{
	free(fatFileSystem.fatTable);
  fclose(FILE_SYSTEM_ID);
  return 0;
}

int loadWorkingDirectory()
{
  char path[FAT12_MAX_PATH_NAME_LENGTH];

  // default to root directory.
  fatFileSystem.workingDirectory.depthLevel = 1;
  fatFileSystem.workingDirectory.dirLevels[0].indexInParentDirectory = 0;
  fatFileSystem.workingDirectory.dirLevels[0].firstLogicalCluster = 0;
	
  if (access(WORKING_DIRECTORY_FILE_NAME, F_OK) != -1)
  {
    // file exists.
    
    FILE* cwdFile = fopen(WORKING_DIRECTORY_FILE_NAME, "r");

    if (cwdFile == NULL)
	  {
      printf("Could not initialze current working directory.\n");
      return 1;
    }

    // Read the path name from the file.
    fgets(path,
      sizeof(path), cwdFile);

    // Remove the newline character from the end of the path name.
    char *pos = strchr(path, '\n');
    if (pos != NULL)
        *pos = '\0';

    // TODO: Construct the directory levels from the path.
	printf("Path:%s\n", path);
    changeWorkingDirectory(path);

    //printf("pwd = \"%s\"\n", fatFileSystem.workingDirectory.pathName);

    fclose(cwdFile);
  }
  else
  {
    // file does not exist.
    
    // default to root directory.
    strcpy(fatFileSystem.workingDirectory.pathName, "/");
    fatFileSystem.workingDirectory.depthLevel = 1;
    fatFileSystem.workingDirectory.dirLevels[0].indexInParentDirectory = 0;
    fatFileSystem.workingDirectory.dirLevels[0].firstLogicalCluster = 0;

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

int getFatBootSector(FatBootSector* bootSector)
{
  memcpy(bootSector, &fatFileSystem.bootSector, sizeof(FatBootSector));
  return 0;
}

int changeWorkingDirectory(const char* pathName)
{
/*
  if (pathName[0] == '/')
  {

  }
  else
  {
*/
    char* path = fatFileSystem.workingDirectory.pathName;
    char* token;

    char* pathString = (char*) malloc(strlen(pathName) + 1);
    strcpy(pathString, pathName);
    token = strtok(pathString, "/");
    
    int counter = 0;
    DirectoryEntry dir;

    WorkingDirectory *workingDir = &fatFileSystem.workingDirectory;
    DirectoryLevel *dirLevels = &workingDir->dirLevels[workingDir->depthLevel];
    int index;
    
    while (token != NULL)
    {
       strcpy(path, token);
       path += strlen(token);
       *path = '/';
       path++;
       if (findDirectoryEntryByName(dirLevels->firstLogicalCluster, &dir, &index,
                                    token) == 0)
       {
          //we found it
          dirLevels++;
          workingDir->depthLevel++;
          dirLevels->indexInParentDirectory = index;
          dirLevels->firstLogicalCluster = dir.firstLogicalCluster;       
          printf("Directory found: %s, %i, %i\n", token, index, dirLevels->firstLogicalCluster);   
       }
       else
       {
          printf("Directory %s doesn't exist\n", token);
          return 1;
       }
       token = strtok(NULL, "/");
       counter++;       
    }    
    *(path-1) = '\0';    
  //}
   saveWorkingDirectory();
  return 0;
}

const char* getWorkingDirectoryPathName()
{
  return fatFileSystem.workingDirectory.pathName;
}

int readLogicalClusterChain(int firstLogicalCluster,
                            unsigned char** dataPtr,
                            unsigned int* numBytes)
{ 
  *dataPtr = (unsigned char*) malloc(fatFileSystem.bootSector.bytesPerSector);
  
  // Translate logical cluster number into physical sector number.
  unsigned int physicalSector = firstLogicalCluster;
  if (physicalSector == 0)
    physicalSector = fatFileSystem.sectorOffsets.rootDirectory;
  else
    physicalSector = fatFileSystem.sectorOffsets.dataRegion + firstLogicalCluster - 2;
      
  read_sector(physicalSector, *dataPtr);
  return 0;
}

int findDirectoryEntryByName(int firstLogicalCluster, DirectoryEntry* entry, int* index, const char* name)
{
  unsigned char* dataPtr;
  unsigned int numBytes;
  char fileName[FAT12_MAX_FILE_NAME_LENGTH];
  readLogicalClusterChain(firstLogicalCluster, &dataPtr, &numBytes);
  DirectoryEntry* dir = (DirectoryEntry*) dataPtr; 
  
  int i, k;
  for (k = 0; k < 20; k++)	
  {
    if ((unsigned char) dir[k].name[0] == 0xE5)
    {
      // The directory entry is free (i.e., currently unused).
    }
    else if ((unsigned char) dir[k].name[0] == 0x00)
    {      
      // This directory entry is free and all the remaining directory
      // entries in this directory are also free.
      break;
    }
    else if (dir[k].attributes == 0x0F)
    {
      // Long filename, ignore.
    }
    else
    {                 
      int isSubDir = (dir[k].attributes & 0x10);
      entryToString(&dir[k], fileName);
      if (strcmp(fileName, name) == 0)
      {
        //yay
        *index = k;
        *entry = dir[k];
        return 0;
      }
    }   
  } 
  
 // free(dataPtr);
  return -1;
}

int entryToString(DirectoryEntry* entry, char* string)
{
  memset(string, 0, FAT12_MAX_FILE_NAME_LENGTH);
  // Print the file name.
  int counter = 0;
  int i;
  for (i = 0; i < 8; i++)
  {
    if (entry->name[i] == ' ')
      break;
    string[counter++] = entry->name[i];
   }
   if (entry->extension[0] != ' ')
   {
     string[counter++] = '.';
     for (i = 0; i < 3; i++)
     {
        if (entry->extension[i] == ' ')
          break;
        string[counter++] = entry->extension[i];
     }
   }
  return 0;
}


