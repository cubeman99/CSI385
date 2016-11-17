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
		FilePath filePath;
		getWorkingDirectory(&filePath);	
  		
  	// Change to the given file path.
  	if (changeFilePath(&filePath, argv[1], PATH_TYPE_FILE) != 0)
  	{
  	  return -1; // File not found.
	  }
  		
  	// Get the first logical cluster of this file.
  	unsigned short flc = filePath.dirLevels[filePath.depthLevel - 1].firstLogicalCluster;  
  		
  	// Print out the file contents.
		unsigned char* output;
		unsigned int numBytes;
		if (readFileContents(flc, &output, &numBytes) == 0)
		{
			printf("%s\n", output);
			return 0;
		}
		
  	return -1; // Error reading file data.
	}
	
	terminateFatFileSystem();
}
