/*****************************************************************************
 * cat.c: List information about files.
 *
 * Author: Joey Gallahan
 * 
 * Description: Performs the cat command, which prints the contents of a file 
 *							to the screen. (defaulting to the current working
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
  	// Load the working directory.
		FilePath dirPath;
		getWorkingDirectory(&dirPath);	
		
		//Make a new path to the requested file
  	FilePath newPath;
  	char* test = (char*) malloc(sizeof(dirPath.pathName) + sizeof(argv[1]));
  	strcpy(test, dirPath.pathName);
  	strcat(test, argv[1]);
  		
  	//Change to the new path
  	changeFilePath(&newPath, test, PATH_TYPE_FILE);
  		
  	//Get the first logical cluster of this file
  	unsigned short flc = newPath.dirLevels[newPath.depthLevel - 1].firstLogicalCluster;  
  		
  	//Print out the file contents if there are any
		unsigned char* output;
		unsigned int numBytes;
		if (readFileContents(flc, &output, &numBytes) == 0)
		{
			printf("%s\n", output);
			return 0;
		}
  	printf("Error: File not found.\n");
  	return -1;
	}
}
