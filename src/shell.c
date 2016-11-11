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
   char command[32];
   char* params[32];
   int status;
   int i;
   int exitShell = FALSE;

   while (exitShell != TRUE)
   {
      displayPrompt();
      readCommand(command, params);
      
      // A hard-coded exit command will quit the shell.
      if (strcmp(command, "exit") == 0)
      { 
         exitShell = TRUE;
      }
      else if (access(command, F_OK) == -1)
      {
         printf("Error: Unknown command '%s'\n", command);
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
            execve(command, params, 0);
      	 }
      }
      
      // Free up the previously allocated parameter strings.
      for (i = 0; params[i] != NULL; i++)
      {
         free(params[i]);
      }
   }   

   return 0;
}


void displayPrompt()
{
  printf("Enter a command: ");
}


void readCommand(char* command, char** params)
{
   char* lineOfInput = NULL; // getline() will allocate this string.
   size_t numBytes = 0;
   int counter = 0;
 
   // Get the user's line of input, then tokenize it, delimited by spaces.
   getline(&lineOfInput, &numBytes, stdin);
   char* token = strtok(lineOfInput, " \n");
   
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
}


