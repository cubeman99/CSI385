/*****************************************************************************
 * ls.c: List information about files.
 *
 * Author: Joey Gallahan
 * 
 * Description: Performs the ls command, which lists information about the
 *              files in a directory (defaulting to the current working
 *              directory).
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

void listFileInfo(FilePath* filePath);
void listDirectoryContents(FilePath* filePath);
void printListHeader();
void printEntryInfo(DirectoryEntry* entry);


int main(int argc, char* argv[])
{
  if (initializeFatFileSystem() != 0)
    return -1;
  
  // Validate the number of arguments.
  if (argc > 2)
  {
    printf("Error: Too many arguments.\n");
    return -1;
  }
  
  // Load the working directory.
  FilePath filePath;
  getWorkingDirectory(&filePath);

  // If given a path argument, change to that path.
  if (argc == 2)
  {
    if (changeFilePath(&filePath, argv[1], PATH_TYPE_ANY) != 0)
    {
      return -1;
    }
  }
  
  if (filePath.isADirectory)
  {
    listDirectoryContents(&filePath);
  }
  else if (filePath.depthLevel > 1)
  {
    listFileInfo(&filePath);
  }
  else
  {
    printf("Unknown error with file path.\n");
  }
  
  terminateFatFileSystem();
  return 0;
}


// ls command with file.
void listFileInfo(FilePath* filePath)
{
  // Open the parent directory.
  unsigned short flcOfParent = filePath->dirLevels[
    filePath->depthLevel - 2].firstLogicalCluster;
  DirectoryEntry* parentDir = openDirectory(flcOfParent);

  if (parentDir == NULL)
    return;
  
  // Get the target file in the parent directory.
  DirectoryEntry* entry = parentDir + filePath->dirLevels[
    filePath->depthLevel - 1].indexInParentDirectory;

  // Print info about the target file.
  printListHeader();
  printEntryInfo(entry);

  closeDirectory(parentDir);
}

// ls command with directory.
void listDirectoryContents(FilePath* filePath)
{
  DirectoryEntry* entry;
  int index = 0;
    
  // Open the directory's data.
  unsigned short flc = filePath->dirLevels[
    filePath->depthLevel - 1].firstLogicalCluster;
  DirectoryEntry* directory = openDirectory(flc);
  if (directory == NULL)
    return;
  
  printListHeader();
  
  // Print out info for each entry in the directory.
  for (entry = getFirstValidEntry(directory, &index); entry != NULL;
       entry = getNextValidEntry(entry, &index))
  {
    printEntryInfo(entry);
  }
  
  closeDirectory(directory);
}

void printListHeader()
{
  printf("Name%-10sType%-10sSize%-10sFLC\n", "", "", "");
  printf("---------------------------------------------\n");
}

void printEntryInfo(DirectoryEntry* entry)
{
  char name[FAT12_MAX_FILE_NAME_LENGTH];
  const char* type;
  
  getEntryName(entry, name);

  if (isEntryADirectory(entry))
    type = "Dir";   
  else
    type = "File";

  printf("%-14s%4s%14d%13d\n", name, type, entry->fileSize,
         entry->firstLogicalCluster);  
}
