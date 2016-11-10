/*****************************************************************************
 * Author: David Jordan & Joey Gallahan
 * 
 * Description: TODO
 *
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fat.h"


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------

FatFileSystem fatFileSystem;


//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

static int loadBootSector();
static unsigned char* readFatTable(int fatIndex);
static void freeFatTable(unsigned char* fatTable);


//-----------------------------------------------------------------------------
// FAT12 interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * initializeFatFileSystem
 *****************************************************************************/
int initializeFatFileSystem()
{
  // Open the disk image file.
  fatFileSystem.fileSystemId = fopen(FAT12_DISK_IMAGE_FILE_NAME, "r+");
  if (fatFileSystem.fileSystemId == NULL)
  {
    printf("Could not open the floppy drive or image.\n");
    return -1;
  }

  // Load the boot sector.
  if (loadBootSector() != 0)
  {
    printf("Something has gone wrong -- could not read the boot table\n");
    return -1;
  }

  // Read the first FAT table.
  fatFileSystem.fatTable = readFatTable(0);
  if (fatFileSystem.fatTable == NULL)
  {
    printf("Something has gone wrong -- could not read the FAT table\n");
    return -1;
  }

  return 0;
}

/******************************************************************************
 * terminateFatFileSystem
 *****************************************************************************/
void terminateFatFileSystem()
{
  free(fatFileSystem.fatTable);
  fclose(fatFileSystem.fileSystemId);
}

/******************************************************************************
 * getFatBootSector
 *****************************************************************************/
void getFatBootSector(FatBootSector* bootSector)
{
   memcpy(bootSector, &fatFileSystem.bootSector, sizeof(FatBootSector));
}

/******************************************************************************
 * getNumberOfUsedBlocks
 *****************************************************************************/
void getNumberOfUsedBlocks(int* numUsedBlocks, int* totalBlocks)
{
  *numUsedBlocks = 0;
  *totalBlocks = fatFileSystem.bootSector.totalSectorCount -
                 fatFileSystem.sectorOffsets.dataRegion;
  
  int sector = fatFileSystem.sectorOffsets.dataRegion;
  int entry;
  
  for (; sector < fatFileSystem.bootSector.totalSectorCount; sector++)
  {
    entry = get_fat_entry(sector, fatFileSystem.fatTable);
    if (entry != 0x00)
      (*numUsedBlocks)++;
  }
}


//-----------------------------------------------------------------------------
// File Path interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * initFilePath
 *****************************************************************************/
void initFilePath(FilePath* filePath)
{
  // Initialize to the root directory.
  strcpy(filePath->pathName, "/");
  filePath->isADirectory = 1;
  filePath->depthLevel = 1;
  filePath->dirLevels[0].indexInParentDirectory = 0; // irrelevant for root
  filePath->dirLevels[0].firstLogicalCluster    = 0;
  filePath->dirLevels[0].offsetInPathName       = 1;
}

/******************************************************************************
 * getWorkingDirectory
 *****************************************************************************/
void getWorkingDirectory(FilePath* filePath)
{
  // TODO: Use some sort of shared memory to save this stuff.

  // Always start at the root directory.
  initFilePath(filePath);

  // Load the current working directory from file.
  if (access(WORKING_DIRECTORY_FILE_NAME, F_OK) != -1)
  {
    // The file exists, open it.
    FILE* cwdFile = fopen(WORKING_DIRECTORY_FILE_NAME, "r");
    if (cwdFile == NULL)
    {
      printf("Failure to get current working directory.\n");
      return;
    }

    // Read the path name from the file.
    char pathName[FAT12_MAX_PATH_NAME_LENGTH];
    fgets(pathName, sizeof(pathName), cwdFile);
 
    // Remove the newline character from the end of the path name.
    char *pos = strchr(pathName, '\n');
    if (pos != NULL)
      *pos = '\0';

    // Change the file path using the read path name.
    changeFilePath(filePath, pathName, PATH_TYPE_DIRECTORY);

    fclose(cwdFile);
  }
}

/******************************************************************************
 * setWorkingDirectory
 *****************************************************************************/
void setWorkingDirectory(FilePath* filePath)
{
  FILE* cwdFile = fopen(WORKING_DIRECTORY_FILE_NAME, "w");

  if (cwdFile == NULL)
  {
    printf("Failure to set current working directory.\n");
    return;
  }

  fprintf(cwdFile, "%s\n", filePath->pathName);

  fclose(cwdFile);
  return;
}

/******************************************************************************
 * changeFilePath
 *****************************************************************************/
int changeFilePath(FilePath* filePath, const char* pathName, int pathType)
{
  DirectoryEntry* directory;
  DirectoryEntry entry;
  int index, i;
  
  // Store the new file path in another variable, in case the opeartion
  // doesn't complete successfully.
  FilePath newFilePath = *filePath;
  char* path = newFilePath.pathName;

  if (pathName[0] == '/')
  {
    // pathName is an absolute path (starts with a '/')
    // Reset the starting file path to the root directory.
    initFilePath(&newFilePath);
  }
  else if (newFilePath.depthLevel > 1)
  {
    // pathName is a relative path.
    path += strlen(newFilePath.pathName);
  }

  // Duplicate the pathName string (because strtok() will modify it).
  char* tokenizedPath = (char*) malloc(strlen(pathName) + 1);
  strcpy(tokenizedPath, pathName);
  
  // Get the first token (directory name) from the path string.
  char* token;
  token = strtok(tokenizedPath, "/");
  
  DirectoryLevel *dirLevels = &newFilePath.dirLevels[newFilePath.depthLevel - 1];

  // Handle one token (directory name) after another.
  while (token != NULL)
  {
    // Can't increase depth if we've reached a file instead of a directory.
    if (!newFilePath.isADirectory)
    {
      printf("%s: Not a directory\n", pathName);
      return -2;
    }
  
    // Find the entry by token name in the top-most directory.
    directory = openDirectory(dirLevels->firstLogicalCluster);
    index = findEntryByName(directory, token);
    if (index >= 0)
      entry = directory[index];
    closeDirectory(directory);
    
    if (index < 0)
    {
      printf("%s: No such file or directory\n", pathName);
      return -1;
    }
    else
    {
      // We found the next entry.
      dirLevels++;
      dirLevels->firstLogicalCluster = entry.firstLogicalCluster;
      dirLevels->indexInParentDirectory = index;
      if (newFilePath.depthLevel == 1)
        dirLevels->offsetInPathName = 1;
      else
        dirLevels->offsetInPathName = strlen(newFilePath.pathName) + 1;
      newFilePath.depthLevel++;
      newFilePath.isADirectory = isEntryADirectory(&entry);

      // Add the directory name to the absolute path name.
      *path = '/';
      path++;
      strcpy(path, token);
      path += strlen(token);

      // Remove any path redundancies (. and ..) based on matching FLCs.
      for (i = 0; i < newFilePath.depthLevel - 1; i++)
      {
        if (entry.firstLogicalCluster ==
            newFilePath.dirLevels[i].firstLogicalCluster)
        {
          newFilePath.depthLevel = i + 1;
          dirLevels = &newFilePath.dirLevels[i];
          
          if (i > 0)
          {
            path = newFilePath.pathName + newFilePath.dirLevels[i + 1]
                   .offsetInPathName - 1;
            *path = '\0';
          }
          else
          {
            path = newFilePath.pathName;
            path[1] = '\0';
          }
          
          break;
        }
      }
    }

    // Move onto the next token (directory name) in the given path name.
    token = strtok(NULL, "/");
  }
  
  // Validate the file type.
  if (newFilePath.isADirectory && pathType == PATH_TYPE_FILE)
  {
    printf("%s: Is a directory\n", pathName);
    return -2;
  }
  else if (!newFilePath.isADirectory && pathType == PATH_TYPE_DIRECTORY)
  {
    printf("%s: Not a directory\n", pathName);
    return -2;
  }

  // Success!
  *filePath = newFilePath;
  return 0;
}


//-----------------------------------------------------------------------------
// Directory interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * openDirectory
 *****************************************************************************/
DirectoryEntry* openDirectory(unsigned short flc)
{
  unsigned char* data;
  unsigned int numBytes;
  
  if (readFileContents(flc, &data, &numBytes) == 0)
  {
    return (DirectoryEntry*) data;
  }
  else
  {
    return NULL;
  }
}

/******************************************************************************
 * closeDirectory
 *****************************************************************************/
void closeDirectory(DirectoryEntry* directory)
{
  free(directory);
}

/******************************************************************************
 * saveDirectory
 *****************************************************************************/
int saveDirectory(unsigned short flc, DirectoryEntry* directory)
{
  // TODO: Implement for the 'rm', 'rmdir', 'touch', and 'mkdir' commands.
}

/******************************************************************************
 * organizeDirectory
 *****************************************************************************/
void organizeDirectory(DirectoryEntry* directory)
{
  // TODO: Implement for the 'rm' and 'rmdir' commands.
}

/******************************************************************************
 * removeEntry
 *****************************************************************************/
void removeEntry(DirectoryEntry* directory, int index)
{
  // TODO: Implement for the 'rm' and 'rmdir' commands.
}

/******************************************************************************
 * findEntryByName
 *****************************************************************************/
int findEntryByName(DirectoryEntry* directory, const char* name)
{
  char entryName[FAT12_MAX_FILE_NAME_LENGTH];
  DirectoryEntry* entry;
  int index = 0;
  
  // Check each entry in the directory.
  for (entry = getFirstValidEntry(directory, &index); entry != NULL;
       entry = getNextValidEntry(entry, &index))
  {
    getEntryName(entry, entryName);
    
    // Check if we found the entry.
    if (strcasecmp(entryName, name) == 0)
    {
      return index;
    }
  }
  
  return -1;
}

/******************************************************************************
 * getFirstValidEntry
 *****************************************************************************/
DirectoryEntry* getFirstValidEntry(DirectoryEntry* entry, int* indexCounter)
{
  while (1)
  {
    if ((unsigned char) entry->name[0] == DIR_ENTRY_FREE)
    {
      // The directory entry is free (i.e., currently unused).
    }
    else if ((unsigned char) entry->name[0] == DIR_ENTRY_END_OF_ENTRIES)
    {      
      // This directory entry is free and all the remaining directory
      // entries in this directory are also free.
      return NULL;
    }
    else if (entry->attributes == DIR_ENTRY_ATTRIB_LONG_FILE_NAME)
    {
      // Long filename, ignore.
    }
    else
    {
      // We found a valid entry!
      return entry;
    }
    
    entry++; // Check the next entry.
    
    if (indexCounter != NULL)
      (*indexCounter)++;
  }
  
  return NULL;
}

/******************************************************************************
 * getNextValidEntry
 *****************************************************************************/
DirectoryEntry* getNextValidEntry(DirectoryEntry* entry, int* indexCounter)
{
  if (indexCounter != NULL)
    (*indexCounter)++;
  entry++;
  return getFirstValidEntry(entry, indexCounter);
}

/******************************************************************************
 * isDirectoryEmpty
 *****************************************************************************/
int isDirectoryEmpty(DirectoryEntry* directory)
{
  char name[FAT12_MAX_FILE_NAME_LENGTH];
  DirectoryEntry* entry;
  int index = 0;
  
  // Check each entry in the directory.
  for (entry = getFirstValidEntry(directory, &index); entry != NULL;
       entry = getNextValidEntry(entry, &index))
  {
    getEntryName(entry, name);
    
    // Check if this entry is not . or ..
    if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
    {
      return 0;
    }
  }
  
  return 1; // The directory is either empty or contains only . and ..
}

/******************************************************************************
 * createNewEntry
 *****************************************************************************/
int createNewEntry(unsigned short flc, DirectoryEntry** directory,
                   const char* name, int* newEntryIndex)
{
  // TODO: Implement for the 'touch' and 'mkdir' commands.
}


//-----------------------------------------------------------------------------
// Directory Entry interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * isEntryADirectory
 *****************************************************************************/
int isEntryADirectory(DirectoryEntry* entry)
{
  if (entry->attributes & DIR_ENTRY_ATTRIB_SUBDIRECTORY)
    return 1;
  else
    return 0;
}

/******************************************************************************
 * getEntryName
 *****************************************************************************/
void getEntryName(DirectoryEntry* entry, char* nameString)
{
  const int nameLength = 8;
  const int extLength = 3;
  int counter = 0;
  int i;

  // Grab the name.
  for (i = 0; i < nameLength; i++)
  {
    if (entry->name[i] == ' ')
      break;
    nameString[counter++] = entry->name[i];
  }

  // Grab the extension.
  if (entry->extension[0] != ' ')
  {
    nameString[counter++] = '.';
    for (i = 0; i < extLength; i++)
    {
      if (entry->extension[i] == ' ')
        break;
      nameString[counter++] = entry->extension[i];
    }
  }
  
  // Null terminate the string.
  nameString[counter] = '\0';
}


//-----------------------------------------------------------------------------
// File Data interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * readFileContents
 *****************************************************************************/
int readFileContents(unsigned short flc, unsigned char** data,
                     unsigned int* numBytes)
{
  unsigned int numSectors = 1;
  
  // TODO: Count the number of sectors in the cluster chain.
  
  *numBytes = numSectors * fatFileSystem.bootSector.bytesPerSector;
  *data = (unsigned char*) malloc(*numBytes);
  
  // TODO: Make this actually follow the cluster chain instead of reading only
  // the first cluster.
  
  // Translate logical cluster number into physical cluster number.
  unsigned int physicalCluster = flc;
  if (physicalCluster == 0)
    physicalCluster = fatFileSystem.sectorOffsets.rootDirectory;
  else
    physicalCluster = fatFileSystem.sectorOffsets.dataRegion + flc - 2;
  
  read_sector(physicalCluster, *data);

  return 0;
}

/******************************************************************************
 * writeFileContents
 *****************************************************************************/
int writeFileContents(unsigned short flc, unsigned char* data,
                      unsigned int numBytes)
{
  // TODO: Implement for the 'touch' and 'mkdir' commands.
}

/******************************************************************************
 * freeFileContents
 *****************************************************************************/
int freeFileContents(unsigned short flc)
{
  // TODO: Implement this for the 'rm' and 'rmdir' commands.
}


//-----------------------------------------------------------------------------
// Internal Functions
//-----------------------------------------------------------------------------

/******************************************************************************
 * loadBootSector
 *****************************************************************************/
static int loadBootSector()
{
  // Make sure we're at the beginning of the disk image file.
  if (fseek(fatFileSystem.fileSystemId, 0, SEEK_SET) != 0)
  {
    return -1;
  }

  // Read the boot sector from the disk image file.
  int bytesRead = fread(&fatFileSystem.bootSector, sizeof(char),
                        sizeof(FatBootSector), fatFileSystem.fileSystemId);
  if (bytesRead != sizeof(FatBootSector))
  {
    return -1;
  }

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
 * readFatTable
 *****************************************************************************/
static unsigned char* readFatTable(int fatIndex)
{
  // Allocate the FAT buffer.
  unsigned char* buffer = (unsigned char*) malloc(fatFileSystem.bootSector
    .bytesPerSector * fatFileSystem.bootSector.sectorsPerFAT);
  
  // Calculate the table's sector number.
  unsigned int sector = fatFileSystem.sectorOffsets.fatTables +
                        (fatIndex * fatFileSystem.bootSector.sectorsPerFAT);
  
  // Read the FAT table from the file.
  if (read_sector(sector, buffer) == -1)
  {
    free(buffer);
    return NULL;
  }

  return buffer;
}

/******************************************************************************
 * freeFatTable
 *****************************************************************************/
static void freeFatTable(unsigned char* fatTable)
{
  free(fatTable);
}


