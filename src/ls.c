/*****************************************************************************
 * Author: Joey Gallahan
 * 
 * Description: Performs the ls command
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

int listDirectoryContents(int numArgs, char* path);
void printDir(unsigned int sector, int depth);

int main(int argc, char* argv[])
{
  initializeFatFileSystem();
  listDirectoryContents(argc-1, argv[1]);
  terminateFatFileSystem();
  return 0;
}

//ls command
int listDirectoryContents(int numArgs, char* path)
{
  WorkingDirectory workingDir = fatFileSystem.workingDirectory;
  char* pathName = workingDir.pathName;
  DirectoryLevel levels;
  unsigned char* dataPtr;
  unsigned short firstLogicalCluster;
  unsigned int numBytes;  
  
  // Read the directory's data containing its directory entries.
  DirectoryEntry* dir = (DirectoryEntry*) dataPtr;
  if (numArgs == 1)
  {
    if (changeWorkingDirectory(path) == 0)
    {
      WorkingDirectory newWorkingDir = fatFileSystem.workingDirectory;
      levels = newWorkingDir.dirLevels[newWorkingDir.depthLevel - 1];
      firstLogicalCluster = levels.firstLogicalCluster;
      printDir(firstLogicalCluster, levels.indexInParentDirectory);
      changeWorkingDirectory(pathName);  
    }
  }  
  else if (numArgs > 1)
  {
    printf("Error: Too many arguments.\n");
    return -1;
  }
  else
  {
    if (loadWorkingDirectory() != 0)
    {
      printf("Error: Could not load working directory.\n");
      return -1;
    }
    levels = workingDir.dirLevels[workingDir.depthLevel - 1];
    firstLogicalCluster = levels.firstLogicalCluster;
    printDir(firstLogicalCluster, levels.indexInParentDirectory);
  }
}

void printDir(unsigned int sector, int depth)
{
  unsigned char bytes[BYTES_PER_SECTOR];
  
  char name[9];
  char extension[5];
  char* type;
  unsigned int size;
  unsigned int FLC;
  
  printf("Name%-10sType%-10sSize%-10sFLC\n", "", "", "");
  printf("---------------------------------------------\n");
  
  // Translate logical cluster number into physical sector number.
  unsigned int physicalSector = sector;
  if (physicalSector == 0)
    physicalSector = fatFileSystem.sectorOffsets.rootDirectory;
  else
    physicalSector = fatFileSystem.sectorOffsets.dataRegion + sector - 2;
      
  read_sector(physicalSector, bytes);

  DirectoryEntry* dir = (DirectoryEntry*) bytes;

  // Print the contents of the directory.
  int i, k;
  for (k = 0; k < 20; k++)	
  {    
    
    if (depth >= 16)
    {
      for (i = 0; i < depth; i++)
        printf("   ");
      printf(" (too deep)\n");
      return;
    }

    if ((unsigned char) dir[k].name[0] == 0xE5)
    {
      // The directory entry is free (i.e., currently unused).
    }
    else if ((unsigned char) dir[k].name[0] == 0x00)
    {
      
      // This directory entry is free and all the remaining directory
      // entries in this directory are also free.
      break;
    }
    else if (dir[k].attributes == 0x0F)
    {
      // Long filename, ignore.
    }
    else
    {   
      int isSubDir = (dir[k].attributes & 0x10);
      if (isSubDir)
        type = (char*)"Dir";   
      else
        type = (char*)"File";

      //Get the name of the directory or file
      for (i = 0; i < 8; i++)
      {
        //If it's blank dont do anything
        if (dir[k].name[i] == ' ')
        {
          name[i] = '\0';
          break;
        }
          
        //For the current and parent directories
        if (dir[k].name[0] == '.')
        {
          name[0] = '.';
          if (dir[k].name[1] == '.')
          {
            name[1] = '.';
          }
          break;
        }
        else
        {
          name[i] = dir[k].name[i];
        }        
      }
      
      FLC = dir[k].firstLogicalCluster; //Get the FLC  
      size = dir[k].fileSize; //Get the size of the file   
         
      //Get the extension if there is one
      if (dir[k].extension[0] != ' ')
      {
        extension[0] = '.';
        for (i = 0; i < 3; i++)
        {
          if (dir[k].extension[i] == ' ')
            break;
          extension[i+1] = dir[k].extension[i];
        }    
        
        extension[4] = '\0';
        
        //Put the name and extension together
        char* nameAndExtension = (char*)malloc(strlen(name) + strlen(extension));
        strcpy(nameAndExtension, name);
        strcat(nameAndExtension, extension); 
        //Print Everything
        printf("%-14s%4s%14d%13d\n", nameAndExtension, type, size, FLC);    
      }
      else
      {
        name[8] = '\0';
        //Print Everything
        printf("%-14s%4s%14d%13d\n", name, type, size, FLC);
      }      
    }
  } 
}

