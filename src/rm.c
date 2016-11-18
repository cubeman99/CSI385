/*****************************************************************************
 * rm.c: Remove a file.
 *
 * Author: Joey Gallahan
 * 
 * Description: Performs the rm command, which removes a file from the target 
 *              directory.
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

void rmCommand(char* pathName);

int main(int argc, char* argv[])
{
  if (initializeFatFileSystem() != 0)
    return -1;
  
  // Validate the number of arguments.
  if (argc > 2)
  {
    printf("Error: Too many arguments. rm only takes 1 argument.\n");
    return -1;
  }
  else if (argc == 1)
  {
  	printf("Error: Too little arguments. rm requires 1 argument.\n");
  	return -1;
  }
  else if (argc == 2)
  {   	
  	rmCommand(argv[1]);  
	}
	
	terminateFatFileSystem();
}

void rmCommand(char* pathName)
{    
  // Load the current working directory.  
  FilePath filePath;
  getWorkingDirectory(&filePath);
  
  // Locate the file's parent directory.
  if (changeFilePath(&filePath, pathName, PATH_TYPE_FILE) != 0)
    return;
  
  // Open the parent directory.
  unsigned short flcOfParentDir = filePath.dirLevels[filePath.depthLevel - 2]
                                  .firstLogicalCluster;
  DirectoryEntry* parentDir = openDirectory(flcOfParentDir); 
  
  int index = filePath.dirLevels[filePath.depthLevel - 1].indexInParentDirectory;
	
	// Remove the file
	removeEntry(parentDir, index);
  organizeDirectory(parentDir);
  saveDirectory(flcOfParentDir, parentDir);
}

