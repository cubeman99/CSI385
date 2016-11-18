/*****************************************************************************
 * rmdir.c: Remove a directory.
 *
 * Author: Joey Gallahan
 * 
 * Description: Performs the rmdir command, which removes an empty directory.
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

void rmdirCommand(char* pathName);

int main(int argc, char* argv[])
{
  if (initializeFatFileSystem() != 0)
    return -1;
  
  // Validate the number of arguments.
  if (argc > 2)
  {
    printf("Error: Too many arguments. rmdir only takes 1 argument.\n");
    return -1;
  }
  else if (argc == 1)
  {
  	printf("Error: Too little arguments. rmdir requires 1 argument.\n");
  	return -1;
  }
  else if (argc == 2)
  {   	
  	rmdirCommand(argv[1]);  
	}
	
	terminateFatFileSystem();
}

void rmdirCommand(char* pathName)
{    
  // Load the current working directory.  
  FilePath dirPath;
  getWorkingDirectory(&dirPath);
  
  // Locate the parent directory.
  if (changeFilePath(&dirPath, pathName, PATH_TYPE_DIRECTORY) != 0)
    return;
  
  // Make sure we're not trying to removing the root directory.
  if (dirPath.dirLevels[dirPath.depthLevel - 1].firstLogicalCluster == 0)
  {
    printf("Error: Cannot remove the root directory.\n");
    return;
  }
    
  // Open the directory's parent directory.
  unsigned short flcOfParentDir = dirPath.dirLevels[dirPath.depthLevel - 2]
                                  .firstLogicalCluster;
  DirectoryEntry* parentDir = openDirectory(flcOfParentDir); 
  
  int index = dirPath.dirLevels[dirPath.depthLevel - 1].indexInParentDirectory;
	
	// Check if the directory-to-remove is empty.
	DirectoryEntry* toDelete = openDirectory(parentDir[index].firstLogicalCluster);
	int isEmpty = isDirectoryEmpty(toDelete);
  closeDirectory(toDelete);
  
  getWorkingDirectory(&dirPath);
  
  // Make sure we are not removing the current working directory.
  if (dirPath.dirLevels[dirPath.depthLevel - 1].firstLogicalCluster == 
     parentDir[index].firstLogicalCluster)
  {
    printf("Error: Cannot remove the current directory.\n");
  }
  else  if (!isEmpty)
	{
    printf("Error: The directory must be empty to delete it.\n");
  }
  else
  {
    // Remove the directory.
	  removeEntry(parentDir, index);
    organizeDirectory(parentDir);
    saveDirectory(flcOfParentDir, parentDir);
  }
  
  closeDirectory(parentDir);
}

