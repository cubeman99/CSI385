/******************************************************************************
 * pwd.c: Print working directory.
 *
 * Author: David Jordan & Joey Gallahan
 *
 * Description: Performs the pwd command, which prints the absolute path name
 *              of the current working directory.
 * 
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fat.h"

int mkdirCommand(char* pathName);

int main(int argc, char* argv[])
{   
  if (argc != 2)
  {
    printf("Error: invalid number of arguments\n");
    printf("Usage: mkdir [PATH]\n");
    return -1;
  }
  
  if (initializeFatFileSystem() != 0)
    return -1;
  
  int rc =  mkdirCommand(argv[1]);
  
  terminateFatFileSystem();
  return rc;
}


int mkdirCommand(char* pathName)
{
  char* directoryName = pathName;
  
  // Load the current working directory.  
  FilePath filePath;
  getWorkingDirectory(&filePath);
  
  // Make sure the file doesn't already exist.
  // TODO Supress this output.
  if (changeFilePath(&filePath, pathName, PATH_TYPE_ANY) == 0)
  {
    printf("Error: cannot create directory '%s': File exists\n", pathName);
    return -1;
  }
  
  // Separate the directory name from the path name.
  char* finalSlash = strrchr(pathName, '/');
  if (finalSlash != NULL)
  {
    directoryName = (char*) malloc(strlen(finalSlash + 1) + 1);
    strcpy(directoryName, finalSlash + 1);
    
    if (finalSlash == pathName)
      finalSlash[1] = '\0';
    else
      finalSlash[0] = '\0';
  }
  else
  {
    directoryName = (char*) malloc(strlen(pathName) + 1);
    strcpy(directoryName, pathName);
    pathName[0] = '\0';
  }
  
  // Locate the file path.
  if (changeFilePath(&filePath, pathName, PATH_TYPE_DIRECTORY) != 0)
  {
    free(directoryName);
    return -1;
  }
    
  // Open the parent directory.
  unsigned short flcOfParentDir = filePath.dirLevels[filePath.depthLevel - 1]
    .firstLogicalCluster;
  DirectoryEntry* parentDir = openDirectory(flcOfParentDir); 
  
  // Create an entry in the parent directory for the new subdirectory.
  int newEntryIndex;
  int rc = createNewEntry(flcOfParentDir, &parentDir, directoryName,
                          &newEntryIndex);
  if (rc != 0)
  {
    free(directoryName);
    return rc;
  }

  DirectoryEntry* dirEntry = &parentDir[newEntryIndex];
  dirEntry->attributes |= DIR_ENTRY_ATTRIB_SUBDIRECTORY;
  dirEntry->fileSize = 0;
  
  // Open the new subdirectory
  DirectoryEntry* directory = openDirectory(dirEntry->firstLogicalCluster);
  
  // Create the '.' entry.
  memset(directory[0].name, ' ', sizeof(directory[0].name));
  memset(directory[0].extension, ' ', sizeof(directory[0].extension));
  directory[0].name[0] = '.';
  directory[0].attributes = DIR_ENTRY_ATTRIB_SUBDIRECTORY;
  directory[0].firstLogicalCluster = dirEntry->firstLogicalCluster;
  directory[0].fileSize = 0;
  
  // Create the '..' entry.
  memset(directory[1].name, ' ', sizeof(directory[1].name));
  memset(directory[1].extension, ' ', sizeof(directory[1].extension));
  directory[1].name[0] = '.';
  directory[1].name[1] = '.';
  directory[1].attributes = DIR_ENTRY_ATTRIB_SUBDIRECTORY;
  directory[1].firstLogicalCluster = flcOfParentDir;
  directory[1].fileSize = 0;
  
  // Terminate the subdirectory.
  directory[2].name[0] = DIR_ENTRY_END_OF_ENTRIES;
  
  // Close the subdirectory.
  saveDirectory(dirEntry->firstLogicalCluster, directory);
  closeDirectory(directory);
  
  // Close the parent directory.
  saveDirectory(flcOfParentDir, parentDir);
  closeDirectory(parentDir);
  
  free(directoryName);
}


