/*****************************************************************************
 * rmdir.c: List information about files.
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
  
  // Open the parent directory.
  unsigned short flcOfParentDir = dirPath.dirLevels[dirPath.depthLevel - 2]
                                  .firstLogicalCluster;
  DirectoryEntry* parentDir = openDirectory(flcOfParentDir); 
  
  int index = dirPath.dirLevels[dirPath.depthLevel - 1].indexInParentDirectory;
	
	DirectoryEntry* toDelete = openDirectory(parentDir[index].firstLogicalCluster);
	
	//Remove the file
	if (isDirectoryEmpty(toDelete))
	{
	  removeEntry(parentDir, index);
    saveDirectory(flcOfParentDir, parentDir);
  }
  else
  {
    printf("Error: The directory must be empty to delete it.\n");
    return;
  }
}

