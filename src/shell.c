/*****************************************************************************
 * shell.c: Runs the shell
 * 
 * Author: David Jordan & Joey Gallahan
 * 
 * Description: Runs the shell, which allows the user to enter commands 
 *              pertaining to a disk structured with the FAT12 file system.
 *
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "fat.h"

#define FALSE 0
#define TRUE 1

void displayPrompt();
int readCommand(char* command, char** params);


int main(int argc, char** argv)
{
   char pathToCommands[512];
   char pathToSpecificCommand[512];
   char commandName[32];
   char* params[32];
   
   int status;
   int i;
   int exitShell = FALSE;
   
   // Validate the number of arguments.
   if (argc > 2)
   {
      printf("Error: Too many arguments!\n");
      printf("Usage: shell [DISK_IMAGE_FILE_PATH]\n");
      return -1;
   }
   
   // Get the file name for the disk image, and make sure it exists.
   const char* diskImageFileName = "../disks/floppy2"; // default file name.
   if (argc == 2)
     diskImageFileName = argv[1];
     
   if (access(diskImageFileName, F_OK) == -1)
   {
      printf("Error: %s: unable to open disk image file\n", diskImageFileName);
      printf("Please provide a path to a disk image file as an argument\n");
      printf("Usage: shell [DISK_IMAGE_FILE_PATH]\n");
      return -1;
   }
   
   // Get the path to the directory where this shell executable is located.
   // This is also where the command executables are located.
   strcpy(pathToCommands, argv[0]);
   char* finalSlash = strrchr(pathToCommands, '/');
   if (finalSlash != NULL)
      finalSlash[1] = '\0';
      
   // Create the shared memory.
   key_t key = FAT12_SHARED_MEMORY_KEY;
   fatFileSystem.sharedMemoryId = shmget(key, 1024, 0644 | IPC_CREAT);
   if (fatFileSystem.sharedMemoryId == -1)
   {
     perror("Error creating shared memory segment");
     return -1;
   }
   fatFileSystem.sharedMemoryPtr = shmat(fatFileSystem.sharedMemoryId, (void *) 0, 0);
   if (fatFileSystem.sharedMemoryPtr == (void*) -1)
   {
     perror("Error attaching shared memory segment");
     return -1;
   }
   fatFileSystem.diskImageFileName = fatFileSystem.sharedMemoryPtr;
   fatFileSystem.workingDirectoryPathName = fatFileSystem.sharedMemoryPtr + 512;
   
   // Initialize the disk image path and current working directory.
   strcpy(fatFileSystem.workingDirectoryPathName, "/");
   strcpy(fatFileSystem.diskImageFileName, diskImageFileName);
   
   // Run the shell's main loop.
   while (exitShell != TRUE)
   {
      displayPrompt();
      if (readCommand(commandName, params) != 0)
         continue;
            
      // Create a path to the command.
      strcpy(pathToSpecificCommand, pathToCommands);
      strcat(pathToSpecificCommand, commandName);
      
      // A hard-coded exit command will quit the shell.
      if (strcmp(commandName, "exit") == 0)
      { 
         exitShell = TRUE;
      }
      else if (access(pathToSpecificCommand, F_OK) == -1)
      {
         printf("Error: Unknown command '%s'\n", commandName);
      }
      else
      {
         // Fork a child process to run the command executable.
         if(fork() != 0)
         {
            waitpid(-1, &status, 0);
         }
         else
         {
            execve(pathToSpecificCommand, params, 0);
      	 }
      }
      
      // Free up the previously allocated parameter strings.
      for (i = 0; params[i] != NULL; i++)
      {
         free(params[i]);
      }
   }
   
   // Destroy the shared memory.
   shmctl(fatFileSystem.sharedMemoryId, IPC_RMID, NULL);
   return 0;
}


void displayPrompt()
{
  printf("Enter a command: ");
}


int readCommand(char* command, char** params)
{
   char* lineOfInput = NULL; // getline() will allocate this string.
   size_t numBytes = 0;
   int counter = 0;
 
   // Get the user's line of input, then tokenize it, delimited by spaces.
   getline(&lineOfInput, &numBytes, stdin);
   char* token = strtok(lineOfInput, " \n");
   
   if (token == NULL)
   {
     // The user entered nothing at all!
     return 1;
   }
   
   // The first token is always the command name.
   strcpy(command, token);

   // Read each parameter.
   while (token != NULL)
   {
      params[counter] = (char*) malloc(strlen(token) + 1);
      strcpy(params[counter], token);
      token = strtok(NULL, " \n");
      counter++;
   }
   params[counter] = NULL; // Null terminate the parameter list.
   
   free(lineOfInput);
   return 0;
}


