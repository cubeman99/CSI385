
FAT12 Project Submission for Peer Review

Course:  CSI-385-02
Date:    11/17/2016
Authors: David Jordan & Joey Gallahan

-------------------------------------------------------------------------------
How to build and run:

 * 'cd' into the main project directory (the one containing this Readme file).
 
 * Enter the command 'make' to compile and link the code.
   - The object files will be put in the obj/ folder
   - The executable files will be put in the bin/ folder. This includes one for
     the shell and one for each available command.
     
 * Run executable file 'shell', now located in the bin folder, passing in a
   path to the disk image to load. The 3 disk image files are located in the
   disks/ directory.
   
   example:
      
      $ bin/shell disks/floppy1
   
 * While running the shell, enter a command name followed by any arguments.
   - Currently, the possible commands are:
       1. pbs
       2. pfe [X] [Y]
       3. cd [PATH]
       4. pwd
       5. ls [PATH]
       6. mkdir [PATH]
       7. touch [PATH]
       8. rm [PATH]
       9. rmdir [PATH]
      10. df
      11. cat [PATH]
      12. exit
      
   
