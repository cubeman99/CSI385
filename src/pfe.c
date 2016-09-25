/******************************************************************************
 * pfe.c: Print FAT entry
 *
 * Author: David Jordan
 *
 * Description: Prints the 12-bit FAT entry values representing logical
 *              sectors X to Y.
 * 
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
		return 1;
	}

	// Parse the first argument X.
	char* endptr;
	int x = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0')
	{
		printf("X must be a number\n");
		usage();
		return 1;
	}

	// Parse the second argument Y.
	int y = strtol(argv[2], &endptr, 10);
	if (*endptr != '\0')
	{	
		printf("Y must be a number\n");
		usage();
		return 1;
	}
	
	// Validate the two numbers X and Y.
	if (x < 2)
	{
		printf("X must be 2 or larger\n");
		usage();
		return 1;
	}
	else if (y < x)
	{
		printf("Y cannot be less than X\n");
		usage();
		return 1;
	}

  // Open the disk image file.
  FILE_SYSTEM_ID = fopen("../disks/floppy1", "r+");
  if (FILE_SYSTEM_ID == NULL)
	{
    printf("Could not open the floppy drive or image.\n");
    return 1;
  }

	// Load the boot sector.
	if (loadFAT12BootSector() != 0)
	{
		printf("Something has gone wrong -- could not read the boot table\n");
		return 1;
	}

	// Read the first FAT table.
	unsigned char* fat = readFAT12Table(0);
  if (fat == NULL)
	{
    printf("Something has gone wrong -- could not read the FAT table\n");
		return 1;
	}
	
	// Print out the FAT entries.
	for (int i = x; i < y; i++)
	{
		int entry = get_fat_entry((unsigned int) i + 1, fat);
		printf("Entry %d: %X\n", i + 1, entry);
	}

	freeFAT12Table(fat);
  return 0;
}

