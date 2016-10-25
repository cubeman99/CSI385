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
  // Always start at the root directory.
  fatFileSystem.workingDirectory.depthLevel = 1;
  fatFileSystem.workingDirectory.dirLevels[0].indexInParentDirectory = 0;
  fatFileSystem.workingDirectory.dirLevels[0].firstLogicalCluster = 0;
  fatFileSystem.workingDirectory.dirLevels[0].offsetInPathName = 1;
  strcpy(fatFileSystem.workingDirectory.pathName, "/");
	
	// Load the current working directory from file.
  if (access(WORKING_DIRECTORY_FILE_NAME, F_OK) != -1)
  {
    // The file exists, open it.
    FILE* cwdFile = fopen(WORKING_DIRECTORY_FILE_NAME, "r");
    if (cwdFile == NULL)
	  {
      printf("Could not initialze current working directory.\n");
      return -1;
    }

    // Read the path name from the file.
    char path[FAT12_MAX_PATH_NAME_LENGTH];
    fgets(path, sizeof(path), cwdFile);

    // Remove the newline character from the end of the path name.
    char *pos = strchr(path, '\n');
    if (pos != NULL)
      *pos = '\0';

    // Change the working directory using the read path name.
    changeWorkingDirectory(path);

    fclose(cwdFile);
  }
  else
  {
    // The file does not exist, save a new one.
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
  WorkingDirectory newWorkingDir = fatFileSystem.workingDirectory;

  char* path = newWorkingDir.pathName;

  if (pathName[0] == '/')
  {
    // Absolute path: start at the root directory.
    newWorkingDir.depthLevel = 1;
    newWorkingDir.dirLevels[0].indexInParentDirectory = 0;
    newWorkingDir.dirLevels[0].firstLogicalCluster = 0;
    newWorkingDir.dirLevels[0].offsetInPathName = 1;
    strcpy(newWorkingDir.pathName, "/");
  }
  else
  {
    path += strlen(newWorkingDir.pathName);
  }

  char* token;

   char* pathString = (char*) malloc(strlen(pathName) + 1);
   strcpy(pathString, pathName);
   token = strtok(pathString, "/");

   int counter = 0;
   DirectoryEntry dir;

   DirectoryLevel *dirLevels = &newWorkingDir.dirLevels[newWorkingDir.depthLevel - 1];
   int index;
   int rc;
   int i;

   while (token != NULL)
   {
    rc = findDirectoryEntryByName(dirLevels->firstLogicalCluster,
                                  &dir, &index, token);
    
    if (rc != 0)
    {
      printf("%s: No such file or directory\n", pathName);
      return 1;
    } 
    else if (!(dir.attributes & 0x10))
    {
      printf("%s: Not a directory\n", pathName);
      return 2;
    }
    else
    {
       // We found the directory.
       dirLevels++;
       newWorkingDir.depthLevel++;
       dirLevels->firstLogicalCluster = dir.firstLogicalCluster;
       dirLevels->indexInParentDirectory = index;
       dirLevels->offsetInPathName = strlen(newWorkingDir.pathName) + 1;
       
       // Add the directory name to the absolute working dir path name.
       *path = '/';
       path++;
       strcpy(path, token);
       path += strlen(token);

        // TODO: Remove path redundancies (. and ..)
        for (i = 0; i < newWorkingDir.depthLevel - 1; i++)
        {
          if (dir.firstLogicalCluster == newWorkingDir.dirLevels[i].firstLogicalCluster)
          {
            printf("Redundancy %d: ", i);
            printf("%s -> ", newWorkingDir.pathName);
            newWorkingDir.depthLevel = i + 1;
            dirLevels = &newWorkingDir.dirLevels[i];
            path = newWorkingDir.pathName + newWorkingDir.dirLevels[i + 1].offsetInPathName;
            *path = '\0';
            printf("%s\n", newWorkingDir.pathName);
            // printf("%s, %s\n", newWorkingDir.pathName,
            //   newWorkingDir.pathName + newWorkingDir.dirLevels[i + 1].offsetInPathName);
            break;
          }
        }
       
       //printf("Directory found: %s, %i, %i\n", token, index, dirLevels->firstLogicalCluster);   
    }

    token = strtok(NULL, "/");
    counter++;       
   }

  fatFileSystem.workingDirectory = newWorkingDir;
  printf("%s\n", fatFileSystem.workingDirectory.pathName);
  saveWorkingDirectory();
  return 0;
}

const char* getWorkingDirectoryPathName()
{
  return fatFileSystem.workingDirectory.pathName;
}

int readLogicalClusterChain(unsigned short firstLogicalCluster,
                            unsigned char** dataPtr,
                            unsigned int* numBytes)
{ 
  *dataPtr = (unsigned char*) malloc(fatFileSystem.bootSector.bytesPerSector);
  
  // Translate logical cluster number into physical cluster number.
  unsigned int physicalCluster = firstLogicalCluster;
  if (physicalCluster == 0)
    physicalCluster = fatFileSystem.sectorOffsets.rootDirectory;
  else
    physicalCluster = fatFileSystem.sectorOffsets.dataRegion + firstLogicalCluster - 2;
  
  // TODO: Read the whole cluster chain.
  read_sector(physicalCluster, *dataPtr);

  return 0;
}

int findDirectoryEntryByName(unsigned short firstLogicalCluster,
                             DirectoryEntry* entry,
                             int* index,
                             const char* name)
{
  unsigned char* dataPtr;
  unsigned int numBytes;
  char fileName[FAT12_MAX_FILE_NAME_LENGTH];
  int k;
  
  // Read the directory's data containing its directory entries.
  readLogicalClusterChain(firstLogicalCluster, &dataPtr, &numBytes);
  DirectoryEntry* dir = (DirectoryEntry*) dataPtr; 
  
  // Loop through the directory entry's, searching by name.
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
      
      // Check if we found the entry.
      if (strcasecmp(fileName, name) == 0)
      {
        *index = k;
        *entry = dir[k];
        free(dataPtr);
        return 0;
      }
    }   
  } 
  
  free(dataPtr);
  return -1;
}

int entryToString(DirectoryEntry* entry, char* string)
{
  memset(string, 0, FAT12_MAX_FILE_NAME_LENGTH);

  int counter = 0;
  int i;
  const int nameLength = 8;
  const int extLength = 3;

  // Grab the name.
  for (i = 0; i < nameLength; i++)
  {
    if (entry->name[i] == ' ')
      break;
    string[counter++] = entry->name[i];
  }

  // Grab the extension.
  if (entry->extension[0] != ' ')
  {
    string[counter++] = '.';
    for (i = 0; i < extLength; i++)
    {
      if (entry->extension[i] == ' ')
        break;
      string[counter++] = entry->extension[i];
    }
  }

  return 0;
}



