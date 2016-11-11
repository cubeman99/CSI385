/******************************************************************************
 * touch.c: Touch a file
 *
 * Author: David Jordan
 *
 * Description: Performs the touch command, which creates a file.
 * 
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fat.h"

int touchCommand(char* pathName);

int main(int argc, char* argv[])
{   
  if (argc != 2)
  {
    printf("Error: invalid number of arguments\n");
    printf("Usage: touch [PATH]\n");
    return -1;
  }
  
  if (initializeFatFileSystem() != 0)
    return -1;
  
  int rc =  touchCommand(argv[1]);
  
  terminateFatFileSystem();
  return rc;
}


int touchCommand(char* pathName)
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
  
  // Separate the file name from the path name.
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
  
  // Create an entry in the parent directory for the new file.
  int newEntryIndex;
  int rc = createNewEntry(flcOfParentDir, &parentDir, directoryName,
                          &newEntryIndex);
  if (rc != 0)
  {
    free(directoryName);
    return rc;
  }

  DirectoryEntry* dirEntry = &parentDir[newEntryIndex];
  dirEntry->attributes = 0;
  dirEntry->fileSize = 0;
  
  // Close the parent directory.
  saveDirectory(flcOfParentDir, parentDir);
  closeDirectory(parentDir);
  
  free(directoryName);
}


