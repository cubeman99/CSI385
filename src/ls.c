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


void listDirectoryContents(DirectoryEntry* directory);


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
  FilePath dirPath;
  getWorkingDirectory(&dirPath);

  // If given a path argument, change to that path.
  if (argc == 2)
  {
    if (changeFilePath(&dirPath, argv[1], PATH_TYPE_DIRECTORY) != 0)
    {
      return -1;
    }
  }
  
  // Open the directory's data.
  unsigned short flc = dirPath.dirLevels[dirPath.depthLevel - 1].firstLogicalCluster;
  DirectoryEntry* directory = openDirectory(flc);
  
  // Print the directory's contents.
  if (directory != NULL)
  {
    listDirectoryContents(directory);
    closeDirectory(directory);
  }
  
  terminateFatFileSystem();
  return 0;
}


// ls command
void listDirectoryContents(DirectoryEntry* directory)
{
  char name[FAT12_MAX_FILE_NAME_LENGTH];
  DirectoryEntry* entry;
  int index = 0;
  const char* type;
  
  printf("Name%-10sType%-10sSize%-10sFLC\n", "", "", "");
  printf("---------------------------------------------\n");
  
  // Print out info for each entry in the directory.
  for (entry = getFirstValidEntry(directory, &index); entry != NULL;
       entry = getNextValidEntry(entry, &index))
  {
    getEntryName(entry, name);
    
    if (isEntryADirectory(entry))
      type = "Dir";   
    else
      type = "File";
    
    printf("%-14s%4s%14d%13d\n", name, type, entry->fileSize,
           entry->firstLogicalCluster);  
  }
}


