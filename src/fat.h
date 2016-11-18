/*****************************************************************************
 * Author: David Jordan & Joey Gallahan
 * 
 * Description: Function headers and struct/enum definitions for the FAT file
 *              system utility functions.
 *
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 ****************************************************************************/
 
#ifndef _FAT_H_
#define _FAT_H_

#include "fatSupport.h"
#include <stdio.h>


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// The maximum number of characters needed to contain a file name.
#define FAT12_MAX_FILE_NAME_LENGTH 13

// The maximum allowed depth of a file path.
#define FAT12_MAX_DIRECTORY_DEPTH 32

// The maximum number of allowed characters for an absolute file path.
#define FAT12_MAX_PATH_NAME_LENGTH (13 * 32)

// If the first byte of the Filename field is 0xE5, then the directory entry is
// free (i.e., currently unused), and hence there is no file or subdirectory
// associated with the directory entry
#define DIR_ENTRY_FREE  0xE5

// If the first byte of the Filename field is 0x00, then this directory entry
// is free and all the remaining directory entries in this directory are also
// free. 
#define DIR_ENTRY_END_OF_ENTRIES  0x00

// If the Attributes byte is 0x0F, then this directory entry is part of a long
// file name and can be ignored for purposes of this assignment. 
#define DIR_ENTRY_ATTRIB_LONG_FILE_NAME  0x0F

// A key to share memory between command processes.
#define FAT12_SHARED_MEMORY_KEY 888888


//-----------------------------------------------------------------------------
// Type Defines
//-----------------------------------------------------------------------------

/******************************************************************************
 * PathType - Possible file types that a path can point to (used for the
 *            function changeFilePath).
 *****************************************************************************/
typedef enum
{
  PATH_TYPE_ANY       = 0,
  PATH_TYPE_DIRECTORY = 1,
  PATH_TYPE_FILE      = 2,
} PathType;

/******************************************************************************
 * DirectoryEntryAttribs - bit masks for the possible attributes of a
 *                         Directory Entry.
 *****************************************************************************/
typedef enum
{
  DIR_ENTRY_ATTRIB_READ_ONLY    = 0x01,
  DIR_ENTRY_ATTRIB_HIDDEN       = 0x02,
  DIR_ENTRY_ATTRIB_SYSTEM       = 0x04,
  DIR_ENTRY_ATTRIB_VOLUME_LABEL = 0x08,
  DIR_ENTRY_ATTRIB_SUBDIRECTORY = 0x10,
  DIR_ENTRY_ATTRIB_ARCHIVE      = 0x20,
} DirectoryEntryAttribs;

/******************************************************************************
 * FatEntryType - possible types of a FAT entry's value.
 *****************************************************************************/
typedef enum
{
  FAT_ENTRY_TYPE_UNUSED      = 0,
  FAT_ENTRY_TYPE_RESERVED    = 1,
  FAT_ENTRY_TYPE_BAD         = 2,
  FAT_ENTRY_TYPE_LAST_SECTOR = 3,
  FAT_ENTRY_TYPE_NEXT_SECTOR = 4,
} FatEntryType;

#pragma pack(1)

/******************************************************************************
 * FatBootSector - struct containing the data elements in the FAT boot sector.
 *****************************************************************************/
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
} FatBootSector;

/******************************************************************************
 * DirectoryEntry - struct for an entry in a directory, representing a file or
 *                  subdirectory in the file system.
 *****************************************************************************/
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

/******************************************************************************
 * DirectoryLevel - a single level in an absolute file path. A FilePath with
 *                  a depth greater than 1 will have multiple DirectoryLevels,
 *                  one for each part of the path.
 *****************************************************************************/
typedef struct
{
  unsigned int   indexInParentDirectory; // negligable for root directory
  unsigned short firstLogicalCluster;
  unsigned int   offsetInPathName;
} DirectoryLevel;

/******************************************************************************
 * FilePath - Represents an absolute path to a file on the file system, 
 *            containing information on how to navigate to each part of the
 *            path.
 *****************************************************************************/
typedef struct
{
  char           pathName[FAT12_MAX_PATH_NAME_LENGTH]; // absolute path name
  DirectoryLevel dirLevels[FAT12_MAX_DIRECTORY_DEPTH];
  unsigned int   depthLevel; // size of dirLevels, 1 = root directory
  int            isADirectory; // 1 if the file path points to a directory,
                               // 0 if it points to a file
} FilePath;

/******************************************************************************
 * FatFileSystem - struct containing information needed to work with a FAT12
 *                 file system.
 *****************************************************************************/
typedef struct
{
  FILE*            fileSystemId;
  FatBootSector    bootSector;
  unsigned char*   fatTable;
  char*            diskImageFileName;  
  char*            workingDirectoryPathName;
  char*            sharedMemoryPtr;
  int              sharedMemoryId;
  
  struct
  {
    unsigned short fatTables;
    unsigned short rootDirectory;
    unsigned short dataRegion;
  } sectorOffsets;
  
} FatFileSystem;

#pragma pack()


//-----------------------------------------------------------------------------
// FAT12 interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * initializeFatFileSystem - Initialize the FAT file system by loading the
 *                           boot sector, reading the first FAT table, and
 *                           loading the working directory
 *
 * Return - 0 on success, -1 on failure
 *****************************************************************************/
int initializeFatFileSystem();

/******************************************************************************
 * terminateFatFileSystem - Terminate the FAT file sysem, deallocating any of
 *                          its used memory
 *
 * Return - none
 *****************************************************************************/
void terminateFatFileSystem();

/******************************************************************************
 * getFatBootSector - Retreive the information from the FAT file system's
 *                    boot sector
 *
 * bootSector - a pointer to the boot sector struct to write to
 *
 * Return - none
 *****************************************************************************/
void getFatBootSector(FatBootSector* bootSector);

/******************************************************************************
 * getNumberOfUsedBlocks - Get the number of used blocks and the total number
 *                         of blocks in the file system, where a block is a
 *                         logical data cluster
 *
 * numUsedBlocks - the number of blocks currently in-use
 * totalBlocks - the total number of blocks in the file system
 *
 * Return - none
 *****************************************************************************/
void getNumberOfUsedBlocks(unsigned short* numUsedBlocks,
                           unsigned short* totalBlocks);


//-----------------------------------------------------------------------------
// File Path interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * initFilePath - Initialize a file path to the root directory
 *
 * filePath - the file path to initialize
 *
 * Return - none
 *****************************************************************************/
void initFilePath(FilePath* filePath);

/******************************************************************************
 * getWorkingDirectory - Load the current working directory into a file path
 *
 * filePath - the file path to load the working directory into
 *
 * Return - none
 *****************************************************************************/
void getWorkingDirectory(FilePath* filePath);

/******************************************************************************
 * setWorkingDirectory - Set the current working directory to the given file
 *                       path
 *
 * filePath - the file path to set the working directory as
 *
 * Return - none
 *****************************************************************************/
void setWorkingDirectory(FilePath* filePath);

/******************************************************************************
 * changeFilePath - Change the path of a file path, either replacing it if
 *                  given a absolute path, or concatenating it if given a
 *                  relative path. If the resulting path doesn't exist, then
 *                  filePath will remain unchanged and -1 will be returned.
 *
 * filePath - the file path to change
 * pathName - the absolute or relative path name to change the file path to
 * type - the type of file that the path must point to. Possible values:
 *         PATH_TYPE_DIRECTORY - must be a directory
 *         PATH_TYPE_FILE      - must be a file
 *         PATH_TYPE_ANY       - can be either a file or directory
 *
 * Return - 0 on success (the resulting path extists), -1 on failure (the
 *          resulting path doesn't exist)
 *****************************************************************************/
int changeFilePath(FilePath* filePath, const char* pathName, int pathType);


//-----------------------------------------------------------------------------
// Directory interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * openDirectory - Open a directory's contents from the file system
 * 
 * flc - the first logical cluster of the directory to open
 *  
 * Return - a list of directory entries on success, or NULL on failure
 *****************************************************************************/
DirectoryEntry* openDirectory(unsigned short flc);

/******************************************************************************
 * closeDirectory - Close an opened directory
 * 
 * directory - the opened directory to close (a pointer to the directory's
 *             first entry)
 *  
 * Return - none
 *****************************************************************************/
void closeDirectory(DirectoryEntry* directory);

/******************************************************************************
 * saveDirectory - Save a directory's contents to the file system
 * 
 * flc - the first logical cluster to save the directory to.
 * directory - the opened directory to save (a pointer to the directory's
 *             first entry)
 *  
 * Return - 0 on success, -1 if there wasn't enough space to save the entire
 *          directory.
 *****************************************************************************/
int saveDirectory(unsigned short flc, DirectoryEntry* directory);

/******************************************************************************
 * organizeDirectory - Organizing a directory's list of entries by removing
 *                     empty space between entries.
 * 
 * directory - the directory's list of entries to organize
 *  
 * Return - none
 *****************************************************************************/
void organizeDirectory(DirectoryEntry* directory);

/******************************************************************************
 * removeEntry - Remove an entry from a directory
 * 
 * directory - the parent directory of the entry to remove
 * index - the index of the entry in its parent directory's list of entries
 *  
 * Return - none
 *****************************************************************************/
void removeEntry(DirectoryEntry* directory, int index);

/******************************************************************************
 * findEntryByName - Find a DirectoryEntry by name in a given directory
 *
 * directory - the list of directory entries to look through
 * name - the name of the directory entry to look for
 *
 * Return - the found entry's index in the given directory, or -1 if the entry
 *          was not found
 *****************************************************************************/
int findEntryByName(DirectoryEntry* directory, const char* name);

/******************************************************************************
 * getFirstValidEntry - Find the first valid entry in a directory. Use this
 *                      along with getNextValidEntry() to iterate the entries
 *                      of a directory.
 *
 * entry - the current entry in a list of entries
 * indexCounter - counter that gets incremented when moving to the next entry
 *
 * Return - the first valid entry in a directory's list of entries
 *****************************************************************************/
DirectoryEntry* getFirstValidEntry(DirectoryEntry* entry, int* indexCounter);

/******************************************************************************
 * getNextValidEntry - Find the next valid entry in a directory, starting
 *                     after the given entry. Use this to iterate the entries
 *                     of a directory.
 *
 * entry - the current entry in a list of entries
 * indexCounter - counter that gets incremented when moving to the next entry
 *
 * Return - the next valid entry after the given entry
 *****************************************************************************/
DirectoryEntry* getNextValidEntry(DirectoryEntry* entry, int* indexCounter);

/******************************************************************************
 * isDirectoryEmpty - Check if a directory is empty (has no entries other than
 *                    . and ..)
 *
 * directory - the directory's list of entries to check
 *
 * Return - 1 if the directory is empty, 0 if it is not
 *****************************************************************************/
int isDirectoryEmpty(DirectoryEntry* directory);

/******************************************************************************
 * createNewEntry - Find a DirectoryEntry by name in a given directory
 *
 * flc - the first logical cluster of the directory
 * directory - the directory's list of directory entries to create a new entry
 *             in. This will be reallocated if there is not enough room in the
 *             directory.
 * name - the name of the new directory entry to create
 * newEntryIndex - the new entry's index in the directory
 * 
 * Return - 0 on success, -1 if there was not enough room on the file system
 *          for one more entry.
 *****************************************************************************/
int createNewEntry(unsigned short flc, DirectoryEntry** directory,
                   const char* name, int* newEntryIndex);


//-----------------------------------------------------------------------------
// Directory Entry interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * isEntryADirectory - Check if a directory entry is a directory instead of a
 *                     file
 *
 * entry - The directory entry to check
 * 
 * Return - 1 if the entry is a directory, 0 if it is a file
 *****************************************************************************/
int isEntryADirectory(DirectoryEntry* entry);

/******************************************************************************
 * getEntryName - Write the full name & extension of a directory entry into a
 *                null terminated string
 *
 * entry - The directory entry to get the name of
 * nameString - a character array to write the name to. The array must have a
 *              length of at least FAT12_MAX_FILE_NAME_LENGTH.
 * 
 * Return - none
 *****************************************************************************/
void getEntryName(DirectoryEntry* entry, char* nameString);

/******************************************************************************
 * setEntryName - Set the name of a directory entry.
 *
 * entry - The directory entry to set the name for
 * nameString - the desired name of the entry
 * 
 * Return - none
 *****************************************************************************/
void setEntryName(DirectoryEntry* entry, const char* nameString);

//-----------------------------------------------------------------------------
// File Data interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * readFileContents - Read the data of a file's contents given its first
 *                    logical cluster
 *
 * flc - the first logical cluster of the file
 * data - a pointer to the files data
 * numBytes - the number of bytes that were read
 * 
 * Return - 0 on success, -1 on failure
 *****************************************************************************/
int readFileContents(unsigned short flc, unsigned char** data,
                     unsigned int* numBytes);

/******************************************************************************
 * writeFileContents - Write data to a file's contents (overwriting any
 *                     existing file data)
 *
 * flc - the first logical cluster of the file
 * data - the data to write
 * numBytes - the number of bytes to write
 * 
 * Return - 0 on success, -1 on failure (if there wasn't enough space on the
 *          file system
 *****************************************************************************/
int writeFileContents(unsigned short flc, unsigned char* data,
                      unsigned int numBytes);

/******************************************************************************
 * freeFileContents - Free a file's contents, freeing the logical clusters it
 *                    is using
 *
 * flc - the first logical cluster of the file
 * 
 * Return - 0 on success, -1 on failure
 *****************************************************************************/
int freeFileContents(unsigned short flc);


//-----------------------------------------------------------------------------
// FAT Table Interface
//-----------------------------------------------------------------------------

/******************************************************************************
 * getFatEntry - Get the value and type of a FAT entry.
 *
 * entryNumber - the number of the FAT entry (equal to a logical sector number)
 * entryValue - the resulting value of the FAT entry
 * entryType - the resulting type of the FAT entry. Possible types:
 *                - FAT_ENTRY_TYPE_UNUSED
 *                - FAT_ENTRY_TYPE_RESERVED
 *                - FAT_ENTRY_TYPE_BAD
 *                - FAT_ENTRY_TYPE_LAST_SECTOR
 *                - FAT_ENTRY_TYPE_NEXT_SECTOR
 * 
 * Return - none
 *****************************************************************************/
void getFatEntry(unsigned short entryNumber, unsigned short* entryValue,
                int* entryType);

/******************************************************************************
 * setFatEntry - Set the value of a FAT entry.
 *
 * entryNumber - the number of the FAT entry (equal to a logical sector number)
 * entryValue - the value to set for the FAT entry
 * 
 * Return - none
 *****************************************************************************/
void setFatEntry(unsigned short entryNumber, unsigned short entryValue);

/******************************************************************************
 * findUnusedFatEntry - Find an unused FAT entry, getting its entry number.
 *
 * entryNumber - the resulting number of an unused FAT entry (if one was found)
 * 
 * Return - 0 if an unused entry was found, -1 if not.
 *****************************************************************************/
int findUnusedFatEntry(unsigned short* entryNumber);

/******************************************************************************
 * getFatEntryChainLength - Count the number of sectors in a chain of FAT
 *                          entries, starting at the given FLC.
 *
 * firstEntryNumber - The FAT entry number to start at
 * 
 * Return - the length of the entry chain starting at firstEntryNumber
 *****************************************************************************/
unsigned short getFatEntryChainLength(unsigned short firstEntryNumber);


//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

extern FatFileSystem fatFileSystem;


#endif //_FAT_H_

