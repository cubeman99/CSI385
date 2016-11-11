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
static void writeFatTable(int fatIndex, unsigned char* fatTable);
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
  writeFatTable(0, fatFileSystem.fatTable);
  freeFatTable(fatFileSystem.fatTable);
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
void getNumberOfUsedBlocks(unsigned int* numUsedBlocks,
                           unsigned int* totalBlocks)
{
  unsigned int entryNumber;
  unsigned int entryValue;
  unsigned int entryType;
  
  unsigned int totalEntries = fatFileSystem.bootSector.totalSectorCount -
                              fatFileSystem.sectorOffsets.dataRegion + 2;
  
  *totalBlocks = totalEntries - 2;
  *numUsedBlocks = 0;
  
  for (entryNumber = 2; entryNumber < totalEntries; entryNumber++)
  {
    getFatEntry(entryNumber, &entryValue, &entryType);
    if (entryType != FAT_ENTRY_TYPE_UNUSED)
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
  unsigned int entryValue;
  unsigned int entryType;
  unsigned int numSectors;
  unsigned int sectorIndex;
  unsigned char* sectorData;
  
  // Count the number of sectors used by the given FLC.
  getFatEntry(flc, &entryValue, &entryType);
  numSectors = 1;
  
  while (entryType != FAT_ENTRY_TYPE_LAST_SECTOR)
  {
    getFatEntry(entryValue, &entryValue, &entryType);
    numSectors++;
  }
  
  *numBytes = numSectors * fatFileSystem.bootSector.bytesPerSector;
  *data = (unsigned char*) malloc(*numBytes);
  
  // Read the data from each sector.
  entryValue = flc;
  sectorData = *data;
    
  for (sectorIndex = 0; sectorIndex < numSectors; sectorIndex++)
  {
    read_sector(logicalToPhysicalCluster(entryValue), sectorData);
    getFatEntry(entryValue, &entryValue, &entryType);
    sectorData += fatFileSystem.bootSector.bytesPerSector;
  }

  return 0;
}

/******************************************************************************
 * writeFileContents
 *****************************************************************************/
int writeFileContents(unsigned short flc, unsigned char* data,
                      unsigned int numBytes)
{
  unsigned int sectorIndex;
  unsigned int entryNumber;
  unsigned int entryValue;
  unsigned int entryType;
  unsigned int numNeededSectors;
  unsigned int numUsedSectors;
  unsigned int temp;
  
  unsigned short bytesPerSector = fatFileSystem.bootSector.bytesPerSector;
  
  // Count the number of needed sectors to write numBytes.
  numNeededSectors = (numBytes + bytesPerSector - 1) / bytesPerSector;
  if (numNeededSectors == 0)
    numNeededSectors = 1;
  
  printf("writeFileContents: flc = %u, numBytes = %u\n", flc, numBytes);
  
  // Count the current number of sectors used by the given FLC.
  getFatEntry(flc, &entryValue, &entryType);
  numUsedSectors = 1;
  printf("[ %u", flc);
  
  while (entryType == FAT_ENTRY_TYPE_NEXT_SECTOR)
  {
    printf(" -> %u", entryValue);
    getFatEntry(entryValue, &entryValue, &entryType);
    numUsedSectors++;
  }
  printf(" ]\n");
  printf("numNeededSectors = %u\n", numNeededSectors);
  printf("numUsedSectors = %u\n", numUsedSectors);
  
  // Check if there isn't enough available sectors for to write all the data.
  if (numNeededSectors > numUsedSectors)
  {
    unsigned int totalSectors;
    unsigned int numUsedBlocks;
    
    getNumberOfUsedBlocks(&numUsedBlocks, &totalSectors);
    
    unsigned int numAvailableSectors = totalSectors - numUsedBlocks + 
                                       numUsedSectors;
    
    if (numAvailableSectors < numNeededSectors)
    {
      printf("Error: not enough available blocks to write %u bytes\n", numBytes);
      return -1;
    }
  }
  
  // There is enough space, write the data to the sectors.
  entryNumber = flc;
  unsigned int maxNeededUsedSectors = numNeededSectors;
  if (numUsedSectors > numNeededSectors)
    maxNeededUsedSectors = numUsedSectors; 
    
  printf("maxNeededUsedSectors = %u\n", maxNeededUsedSectors);
  
  // Write the data to the needed sectors, and free any uneeded 
  // but previously-used sectors.
  for (sectorIndex = 0; sectorIndex < maxNeededUsedSectors; sectorIndex++)
  {
    getFatEntry(entryNumber, &entryValue, &entryType);
    
    // Write the data into this sector.
    if (sectorIndex < numNeededSectors)
    {
      printf("Writing %u bytes to logical sector %u\n",
             numBytes > bytesPerSector ? bytesPerSector : numBytes,
             entryNumber);
      if (numBytes > 0)
      {
        write_sector(logicalToPhysicalCluster(entryNumber), data, numBytes);
        data += bytesPerSector;
        numBytes -= bytesPerSector;
      }
    }
    else
    {
      printf("Freeing sector %u\n", entryNumber);
      setFatEntry(entryNumber, 0x000);
    }
    
    if (sectorIndex == numNeededSectors - 1)
    {
      // Mark the end of the chain of FAT entries.
      setFatEntry(entryNumber, 0xFFF);
    }
    
    if (entryType == FAT_ENTRY_TYPE_NEXT_SECTOR)
    {
      // Reuse this and the next FAT entry.
      entryNumber = entryValue;
    }
    else if (sectorIndex < numNeededSectors - 1)
    {
      // Allocate a new FAT entry.
      findUnusedFatEntry(&temp);
      setFatEntry(entryNumber, temp);
      entryNumber = temp;
    }
  }
  
  getFatEntry(flc, &entryValue, &entryType);
  printf("[ %u", flc);
  while (entryType != FAT_ENTRY_TYPE_LAST_SECTOR)
  {
    printf(" -> %u", entryValue);
    getFatEntry(entryValue, &entryValue, &entryType);
  }
  printf(" ]\n");
  
  return 0;
}

/******************************************************************************
 * freeFileContents
 *****************************************************************************/
int freeFileContents(unsigned short flc)
{
  // TODO: Implement this for the 'rm' and 'rmdir' commands.
}


//-----------------------------------------------------------------------------
// FAT Table Interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * getFatEntry
 *****************************************************************************/
void getFatEntry(unsigned int entryNumber, unsigned int* entryValue,
                int* entryType)
{
  *entryValue = get_fat_entry(entryNumber, fatFileSystem.fatTable);
  
  if (*entryValue == 0x00)
    *entryType = FAT_ENTRY_TYPE_UNUSED;
  else if (*entryValue >= 0xFF0 && *entryValue <= 0xFF6)
    *entryType = FAT_ENTRY_TYPE_RESERVED;
  else if (*entryValue == 0xFF7)
    *entryType = FAT_ENTRY_TYPE_BAD;
  else if (*entryValue >= 0xFF8 && *entryValue <= 0xFFF)
    *entryType = FAT_ENTRY_TYPE_LAST_SECTOR;
  else  
    *entryType = FAT_ENTRY_TYPE_NEXT_SECTOR;
}

/******************************************************************************
 * setFatEntry
 *****************************************************************************/
void setFatEntry(unsigned int entryNumber, unsigned int entryValue)
{
  set_fat_entry(entryNumber, entryValue, fatFileSystem.fatTable);
}

/******************************************************************************
 * findUnusedFatEntry
 *****************************************************************************/
int findUnusedFatEntry(unsigned int* entryNumber)
{
  unsigned int tempEntryNumber;
  unsigned int entryValue;
  unsigned int entryType;
  
  unsigned int totalEntries = fatFileSystem.bootSector.totalSectorCount -
                              fatFileSystem.sectorOffsets.dataRegion + 2;
  
  for (tempEntryNumber = 0; tempEntryNumber < totalEntries; tempEntryNumber++)
  {
    getFatEntry(tempEntryNumber, &entryValue, &entryType);
    if (entryType == FAT_ENTRY_TYPE_UNUSED)
    {
      *entryNumber = tempEntryNumber;
      return 0;
    }
  }
  
  return -1;
}

/******************************************************************************
 * logicalToPhysicalCluster
 *****************************************************************************/
unsigned int logicalToPhysicalCluster(unsigned int logicalCluster)
{
  if (logicalCluster == 0)
    return fatFileSystem.sectorOffsets.rootDirectory;
  else
    return fatFileSystem.sectorOffsets.dataRegion + logicalCluster - 2;
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
 * writeFatTable
 *****************************************************************************/
static void writeFatTable(int fatIndex, unsigned char* fatTable)
{
  unsigned int i;
  
  // Calculate the table's sector number.
  unsigned int sector = fatFileSystem.sectorOffsets.fatTables +
                        (fatIndex * fatFileSystem.bootSector.sectorsPerFAT);
  
  // Write each sector of the table to disk.
  for (i = 0; i < fatFileSystem.bootSector.sectorsPerFAT; i++)
  {
    write_sector(sector + i, fatTable, fatFileSystem.bootSector.bytesPerSector);
    fatTable += fatFileSystem.bootSector.bytesPerSector;
  }
}

/******************************************************************************
 * freeFatTable
 *****************************************************************************/
static void freeFatTable(unsigned char* fatTable)
{
  free(fatTable);
}


