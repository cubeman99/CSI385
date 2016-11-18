/*****************************************************************************
 * rm.c: List information about files.
 *
 * Author: Joey Gallahan
 * 
 * Description: Performs the rm command, which removes a file from the target 
 *              directory. (defaulting to the current working directory).
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
    printf("Error: Too many arguments. cat only takes 1 argument.\n");
    return -1;
  }
  else if (argc == 1)
  {
  	printf("Error: Too little arguments. cat requires 1 argument.\n");
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
  char* fileName = pathName;
  char* parentPathName = pathName;
  printf("To Delete: %s\n", pathName);
  
  // Load the current working directory.  
  FilePath currentPath;
  getWorkingDirectory(&currentPath);
  
  // Separate the directory name from the path name.
  char* finalSlash = strrchr(pathName, '/');
  if (finalSlash != NULL)
  {
    fileName = (char*) malloc(strlen(finalSlash + 1) + 1);
    
    //Allocate memory for the length of the parent directory path
    int parentPathLen = strlen(pathName) - strlen(finalSlash) + 1;
    parentPathName = (char*) malloc(parentPathLen + 1);    
    
    //Put everything before the final slash into the parent path 
    strncpy(parentPathName, pathName, parentPathLen);
    parentPathName[parentPathLen] = '\0'; //null terminate it
    
    strcpy(fileName, finalSlash + 1);    
    
    if (finalSlash == pathName)
      finalSlash[1] = '\0';
    else
      finalSlash[0] = '\0';
  }
  else
  {
    fileName = (char*) malloc(strlen(pathName) + 1);
    
    //Allocate memory for the length of the parent directory path
    int parentPathLen = strlen(currentPath.pathName) + 1;
    parentPathName = (char*) malloc(strlen(currentPath.pathName) + 2);   
    
    //Put everything before the final slash into the parent path 
    strncpy(parentPathName, currentPath.pathName, parentPathLen);
    
    //Need to put in the slash at the end since it's a relative path and
    //the pathName in a FilePath doesn't have it
    parentPathName[parentPathLen-1] = '/';   
    parentPathName[parentPathLen] = '\0'; //null terminate it
    
    strcpy(fileName, pathName);
    
  }
    
  //Change to the parent path.
  if (changeFilePath(&currentPath, parentPathName, PATH_TYPE_DIRECTORY) != 0)
  {
    free(fileName);
    return;
  }
  printf("Parent path name: %s\n", currentPath.pathName);    
  //Open the parent directory.
  unsigned short flcOfParentDir = currentPath.dirLevels[currentPath.depthLevel - 1]
    .firstLogicalCluster;
  DirectoryEntry* parentDir = openDirectory(flcOfParentDir); 
		
	//Make a new path to the requested file
  FilePath filePath;
  printf("original: %s\n", pathName);
  char* path = pathName;
  //If it's a relative path, add the user's input to the current path
  if (pathName[0] != '/')
  {
		path = (char*) malloc(sizeof(parentPathName) + sizeof(pathName));
		strcpy(path, currentPath.pathName);
		if(currentPath.depthLevel != 1)
		{				
		  strcat(path, "/");
		}
			strcat(path, pathName);		
	}
	
	printf("Full file path: %s\n", path);	
  
  //Change to the new path
  changeFilePath(&filePath, path, PATH_TYPE_FILE); 
	
	int index = filePath.dirLevels[filePath.depthLevel - 1].indexInParentDirectory;
	
	//Remove the file
	removeEntry(parentDir, index);
}

