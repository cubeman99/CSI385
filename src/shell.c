/*****************************************************************************
 * Author: David Jordan & Joey Gallahan
 * 
 * Description: Shell V0.1 
 *
 * Certification of Authenticity:
 * I certify that this assignment is entirely my own work.
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "fat.h"

#define FALSE 0
#define TRUE 1

void displayPrompt();
void readCommand(char* command, char** params);

int main()
{
   initializeFatFileSystem();
  
   char command[32];
   char* params[32];
   int status;
   int exitShell = FALSE;

   while (exitShell != TRUE)
   {
      displayPrompt();
      readCommand(command, params);
      
      if (strcmp(command, "exit") == 0)
      { 
         exitShell = TRUE;
      }
      else
      {
         if(fork() != 0)
         {
            waitpid(-1, &status, 0);
         }
         else
         {
            execve(command, params, 0);
      	 }

         int i;
         for (i = 0; params[i] != NULL; i++)
         {
            free(params[i]);
         }
      }
   }   

   terminateFatFileSystem();
   return 0;
}

void displayPrompt()
{
  //loadWorkingDirectory();
  //TODO: Make the prompt show the working directory.
  // printf("%s $ \n", getWorkingDirectoryPathName());

  printf("Enter a command: ");
}

void readCommand(char* command, char** params)
{
   char* lineOfInput = NULL;
   size_t numBytes;
 
   getline(&lineOfInput, &numBytes, stdin);

   char* token;
   token = strtok(lineOfInput, " \n");
   
   strcpy(command, token);

   int counter = 0;

   while (token != NULL)
   {
      params[counter] = (char*) malloc(strlen(token) + 1);
      strcpy(params[counter], token);
      token = strtok(NULL, " \n");
      counter++;
   }
   params[counter] = NULL;
}


