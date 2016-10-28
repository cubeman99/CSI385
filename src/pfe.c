/******************************************************************************
 * pfe.c: Print FAT entry
 *
 * Author: David Jordan
 *
 * Description: Performs the pfe command, which prints the 12-bit FAT entry
 *              values representing logical sectors X to Y.
 * 
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "fat.h"


/******************************************************************************
 * usage - prints the usage statement.
 *****************************************************************************/
static void usage()
{
	printf("Usage: pfe X Y\n");
	printf("Prints the 12-bit FAT entry values representing logical sectors X to Y.\n");
}


/******************************************************************************
 * main - runs the pfe command.
 *****************************************************************************/
int main(int argc, char* argv[])
{
	// Validate the number of arguments.
	if (argc != 3)
	{
		printf("Invalid number of arguments, expected 2.\n");
		usage();
		return -1;
	}

	// Parse the first argument X.
	char* endptr;
	int x = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0')
	{
		printf("X must be a number\n");
		usage();
		return -1;
	}

	// Parse the second argument Y.
	int y = strtol(argv[2], &endptr, 10);
	if (*endptr != '\0')
	{	
		printf("Y must be a number\n");
		usage();
		return -1;
	}
	
	// Validate the two numbers X and Y.
	if (x < 2)
	{
		printf("X must be 2 or greater\n");
		usage();
		return -1;
	}
	else if (y < x)
	{
		printf("Y cannot be less than X\n");
		usage();
		return -1;
	}
	
  if (initializeFatFileSystem() != 0)
    return -1;
	
	// Print out the FAT entries.
	int i;
	for (i = x; i <= y; i++)
	{
		int entry = get_fat_entry((unsigned int) i, fatFileSystem.fatTable);
		printf("Entry %d: %X\n", i, entry);
	}

  terminateFatFileSystem();
  return 0;
}

