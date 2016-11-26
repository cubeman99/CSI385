/*****************************************************************************
 * Author: David Jordan & Joey Gallahan
 * 
 * Description: Function definitions for the FAT file system utility functions.
 *
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "fat.h"


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------

FatFileSystem fatFileSystem;


//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

/******************************************************************************
 * loadBootSector -load the boot sector into memory.
 *
 * Return - Return - 0 on success, -1 on failure
 *****************************************************************************/
static int loadBootSector();

/******************************************************************************
 * readFatTable - read an entire FAT table into memory.
 *
 * fatIndex - the index of the FAT table to load (because there are multiple
 *            FAT tables)  
 *
 * Return - a pointer to the FAT table's data. this must be freed with
 *****************************************************************************/
static unsigned char* readFatTable(int fatIndex);

/******************************************************************************
 * writeFatTable - write a FAT table to disk.
 *
 * fatIndex - the index of the FAT table to write to
 * unsigned char* fatTable - the FAT table's data to write to disk
 *
 * Return - none
 *****************************************************************************/
static void writeFatTable(int fatIndex, unsigned char* fatTable);

/******************************************************************************
 * freeFatTable - free the allocated memory of a FAT table.
 *
 * fatTable - a pointer to the FAT table's data that was allocated with
 *           readFatTable()
 *
 * Return - none
 *****************************************************************************/
static void freeFatTable(unsigned char* fatTable);

/******************************************************************************
 * logicalToPhysicalCluster - Translate a logical cluster number to a physical
 *                            cluster number.
 *
 * entryNumber - logicalCluster
 * 
 * Return - the corresponding physical cluster number
 *****************************************************************************/
unsigned short logicalToPhysicalCluster(unsigned short logicalCluster);


//-----------------------------------------------------------------------------
// FAT12 interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * initializeFatFileSystem
 *****************************************************************************/
int initializeFatFileSystem()
{
  // Setup the shared memory.
  key_t key = FAT12_SHARED_MEMORY_KEY;
  fatFileSystem.sharedMemoryId = shmget(key, 1024, 0644);
  if (fatFileSystem.sharedMemoryId == -1)
  {
    perror("Error creating shared memory segment");
    return -1;
  }
  fatFileSystem.sharedMemoryPtr = shmat(fatFileSystem.sharedMemoryId, (void *) 0, 0);
  if (fatFileSystem.sharedMemoryPtr == (void*) -1)
  {
    perror("Error attaching shared memory segment");
    return -1;
  }
  fatFileSystem.diskImageFileName = fatFileSystem.sharedMemoryPtr;
  fatFileSystem.workingDirectoryPathName = fatFileSystem.sharedMemoryPtr + 512;

  // Open the disk image file.
  fatFileSystem.fileSystemId = fopen(fatFileSystem.diskImageFileName, "r+");
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
  shmdt(fatFileSystem.sharedMemoryPtr);
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
void getNumberOfUsedBlocks(unsigned short* numUsedBlocks,
                           unsigned short* totalBlocks)
{
  unsigned short entryNumber;
  unsigned short entryValue;
  int entryType;
  
  unsigned short totalEntries = (unsigned short)
    (fatFileSystem.bootSector.totalSectorCount -
    fatFileSystem.sectorOffsets.dataRegion + 2);
  
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
  // Always start at the root directory.
  initFilePath(filePath);

  // Change the file path using the current working directory's path name.
  changeFilePath(filePath, fatFileSystem.workingDirectoryPathName,
                 PATH_TYPE_DIRECTORY);
}

/******************************************************************************
 * setWorkingDirectory
 *****************************************************************************/
void setWorkingDirectory(FilePath* filePath)
{
  strcpy(fatFileSystem.workingDirectoryPathName, filePath->pathName);
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
      if (newFilePath.isADirectory)
      {
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
  unsigned short numSectors = getFatEntryChainLength(flc);
  unsigned int numBytes = numSectors * fatFileSystem.bootSector.bytesPerSector;
  
  writeFileContents(flc, (unsigned char*) directory, numBytes);
}

/******************************************************************************
 * organizeDirectory
 *****************************************************************************/
void organizeDirectory(DirectoryEntry* directory)
{
  DirectoryEntry* entry;
  int isUnused;
  int index;
  int scoochAmount = 0;
  int isEnd = 0;
  
  for (index = 0; !isEnd; index++)
  {
    DirectoryEntry* entry = directory + index;
    
    isEnd    = ((unsigned char) entry->name[0] == DIR_ENTRY_END_OF_ENTRIES);
    isUnused = ((unsigned char) entry->name[0] == DIR_ENTRY_FREE ||
               entry->attributes == DIR_ENTRY_ATTRIB_LONG_FILE_NAME);
    
    if (isUnused)
    {
      scoochAmount++;
    }
    else if (scoochAmount > 0)
    {
       directory[index - scoochAmount] = directory[index];
       directory[index].name[0] = DIR_ENTRY_END_OF_ENTRIES;
    }
  }
}

/******************************************************************************
 * removeEntry
 *****************************************************************************/
void removeEntry(DirectoryEntry* directory, int index)
{
  //Get the entry from the parent directory to remove                            
  DirectoryEntry* entryToRemove = directory + index;
  
  entryToRemove->name[0] = DIR_ENTRY_FREE;
  freeFileContents(entryToRemove->firstLogicalCluster);
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
  int index = 0;
  DirectoryEntry* entry = *directory;

  unsigned short bytesPerSector = fatFileSystem.bootSector.bytesPerSector;
  unsigned short numSectorsForDir = getFatEntryChainLength(flc);
  int maxNumEntries = ((numSectorsForDir * bytesPerSector) /
                      sizeof(DirectoryEntry)) - 1;
  
  // Find an unused entry in the given directory.
  while (index < maxNumEntries)
  {
    if ((unsigned char) entry->name[0] == DIR_ENTRY_FREE)
    {
      // The directory entry is free (i.e., currently unused).
      break;
    }
    else if ((unsigned char) entry->name[0] == DIR_ENTRY_END_OF_ENTRIES)
    {      
      // This directory entry is free and all the remaining directory
      // entries in this directory are also free.
      (entry + 1)->name[0] = DIR_ENTRY_END_OF_ENTRIES;
      break;
    }
    
    entry++;
    index++;
  }
  
  // Increase the size of the directory if it is full.
  if (index == maxNumEntries)
  {
    numSectorsForDir++;
    *directory = (DirectoryEntry*) realloc(*directory, numSectorsForDir * bytesPerSector);
    int rc = writeFileContents(flc, (unsigned char*) *directory,
                               numSectorsForDir * bytesPerSector);
    if (rc != 0)
      return rc;
    
    // TODO: remove this debug print.
    printf("Increased directory size to %u\n", numSectorsForDir);
    
    entry = (*directory) + index;
    (entry + 1)->name[0] = DIR_ENTRY_END_OF_ENTRIES;
  }
  
  // Initialize the entry's information.
  setEntryName(entry, name);
  entry->attributes = 0;
  entry->fileSize = 0;
  
  // Allocate a data sector for the entry.
  entry->firstLogicalCluster = 0;
  findUnusedFatEntry(&entry->firstLogicalCluster);
  setFatEntry(entry->firstLogicalCluster, 0xFFF);

  *newEntryIndex = index;
  return 0;
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

/******************************************************************************
 * setEntryName
 *****************************************************************************/
void setEntryName(DirectoryEntry* entry, const char* nameString)
{
  // Fill the entry's name & extension with spaces.
  memset(entry->name, ' ', sizeof(entry->name));
  memset(entry->extension, ' ', sizeof(entry->extension));
  
  // Split the name and extension by the first dot in the given string.
  const char* startOfName = nameString;
  const char* endOfName   = nameString + sizeof(entry->name);
  const char* startOfExt  = nameString;
  const char* endOfExt    = nameString;
  const char* dot         = strchr(nameString, '.');
  
  if (dot != NULL)
  {
    startOfExt = dot + 1;
    endOfExt = dot + 1 + sizeof(entry->extension);
    if (dot < endOfName)
      endOfName = dot;
  }
  
  // Write the characters into the entry's name.
  const char* c;
  int i;
  for (i = 0, c = startOfName; c != endOfName && *c != '\0'; c++)
    entry->name[i++] = toupper(*c);
  for (i = 0, c = startOfExt; c != endOfExt && *c != '\0'; c++)
    entry->extension[i++] = toupper(*c);
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
  unsigned short entryValue;
  int entryType;
  unsigned short numSectors;
  unsigned short sectorIndex;
  unsigned char* sectorData;
  
  // Count the number of sectors used by the given FLC.
  numSectors = getFatEntryChainLength(flc);
  
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
  unsigned short sectorIndex;
  unsigned short entryNumber;
  unsigned short entryValue;
  int entryType;
  unsigned short numNeededSectors;
  unsigned short numUsedSectors;
  unsigned short temp;
  
  unsigned short bytesPerSector = fatFileSystem.bootSector.bytesPerSector;
  
  // Count the number of needed sectors to write numBytes.
  numNeededSectors = (numBytes + bytesPerSector - 1) / bytesPerSector;
  if (numNeededSectors == 0)
    numNeededSectors = 1;
  
  // Count the current number of sectors used by the existing FLC.
  numUsedSectors = getFatEntryChainLength(flc);

  // Check if there isn't enough available sectors for to write all the data.
  if (numNeededSectors > numUsedSectors)
  {
    unsigned short totalSectors;
    unsigned short numUsedBlocks;
    
    getNumberOfUsedBlocks(&numUsedBlocks, &totalSectors);
    
    unsigned short numAvailableSectors = totalSectors - numUsedBlocks + 
                                         numUsedSectors;
    
    if (numAvailableSectors < numNeededSectors)
    {
      printf("Error: not enough available blocks to write %u bytes\n", numBytes);
      return -1;
    }
  }
  
  entryNumber = flc;
  unsigned short maxNeededUsedSectors = numNeededSectors;
  if (numUsedSectors > numNeededSectors)
    maxNeededUsedSectors = numUsedSectors; 
    
  // Write the data to the needed sectors, and free any uneeded 
  // but previously-used sectors.
  for (sectorIndex = 0; sectorIndex < maxNeededUsedSectors; sectorIndex++)
  {
    getFatEntry(entryNumber, &entryValue, &entryType);
    
    // Write the data into this sector.
    if (sectorIndex < numNeededSectors)
    {
      if (numBytes > 0)
      {
        write_sector(logicalToPhysicalCluster(entryNumber), data, numBytes);
        data += bytesPerSector;
        numBytes -= bytesPerSector;
      }
    }
    else
    {
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
  
  return 0;
}

/******************************************************************************
 * freeFileContents
 *****************************************************************************/
int freeFileContents(unsigned short flc)
{
  unsigned int numSectors = getFatEntryChainLength(flc);
  unsigned int numBytes = numSectors * fatFileSystem.bootSector.bytesPerSector; 
  setFatEntry(flc, FAT_ENTRY_TYPE_UNUSED);
}


//-----------------------------------------------------------------------------
// FAT Table Interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * getFatEntry
 *****************************************************************************/
void getFatEntry(unsigned short entryNumber, unsigned short* entryValue,
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
void setFatEntry(unsigned short entryNumber, unsigned short entryValue)
{
  set_fat_entry((unsigned int) entryNumber,
                (unsigned int) entryValue,
                fatFileSystem.fatTable);
}

/******************************************************************************
 * findUnusedFatEntry
 *****************************************************************************/
int findUnusedFatEntry(unsigned short* entryNumber)
{
  unsigned short tempEntryNumber;
  unsigned short entryValue;
  int entryType;
  
  unsigned short totalEntries = fatFileSystem.bootSector.totalSectorCount -
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
 * getFatEntryChainLength
 *****************************************************************************/
unsigned short getFatEntryChainLength(unsigned short firstEntryNumber)
{
  unsigned short entryValue;
  unsigned short length;
  int entryType;
  
  // Get the first entry.
  getFatEntry(firstEntryNumber, &entryValue, &entryType);
  
  if (entryType != FAT_ENTRY_TYPE_NEXT_SECTOR &&
      entryType != FAT_ENTRY_TYPE_LAST_SECTOR)
  {
    return 0;
  }
  
  // Count the current number of entries in a chain.
  for (length = 1; entryType == FAT_ENTRY_TYPE_NEXT_SECTOR; length++)
  {
    getFatEntry(entryValue, &entryValue, &entryType);
  }
  
  return length;
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

/******************************************************************************
 * logicalToPhysicalCluster
 *****************************************************************************/
unsigned short logicalToPhysicalCluster(unsigned short logicalCluster)
{
  if (logicalCluster == 0)
    return fatFileSystem.sectorOffsets.rootDirectory;
  else
    return fatFileSystem.sectorOffsets.dataRegion + logicalCluster - 2;
}



