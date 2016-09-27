#include "pbs.h"

struct PBS gPBS;
unsigned char* stuff = gPBS.contents;

int main()
{
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
   stuff = (unsigned char*) malloc(BYTES_PER_SECTOR);
   readBootSector();

   return 0;
}

void readBootSector()
{
   int mostSignificantBits, leastSignificantBits;

   read_sector(0, stuff);

   gPBS.bps = BYTES_PER_SECTOR; //Bytes per sector

   // msb is 1 higher because little endian 
   //Number of Reserved Sectors
   mostSignificantBits  = ( ( (int) stuff[15] ) << 8 ) & 0x0000ff00;
   leastSignificantBits =   ( (int) stuff[14] )        & 0x000000ff;
   gPBS.nRS = mostSignificantBits | leastSignificantBits;

   //Number of Root Entries
   mostSignificantBits  = ( ( (int) stuff[18] ) << 8 ) & 0x0000ff00;
   leastSignificantBits =   ( (int) stuff[17] )        & 0x000000ff;
   gPBS.nRE = mostSignificantBits | leastSignificantBits;

   //Total Sector Count
   mostSignificantBits  = ( ( (int) stuff[20] ) << 8 ) & 0x0000ff00;
   leastSignificantBits =   ( (int) stuff[19] )        & 0x000000ff;
   gPBS.tsc = mostSignificantBits | leastSignificantBits;

   //Sectors per FAT
   mostSignificantBits  = ( ( (int) stuff[23] ) << 8 ) & 0x0000ff00;
   leastSignificantBits =   ( (int) stuff[22] )        & 0x000000ff;
   gPBS.spf = mostSignificantBits | leastSignificantBits;

   //Sectors per Track
   mostSignificantBits  = ( ( (int) stuff[25] ) << 8 ) & 0x0000ff00;
   leastSignificantBits =   ( (int) stuff[24] )        & 0x000000ff;
   gPBS.spt = mostSignificantBits | leastSignificantBits;

   //Number of Heads
   mostSignificantBits  = ( ( (int) stuff[27] ) << 8 ) & 0x0000ff00;
   leastSignificantBits =   ( (int) stuff[26] )        & 0x000000ff;
   gPBS.nHD = mostSignificantBits | leastSignificantBits;

   //1 byte things dont need to do this ^
   gPBS.spc = stuff[13];   //sectors per cluster
   gPBS.nFT = stuff[16];   //number of FATs
   gPBS.bsh = stuff[38];   //boot signature (in hex)

   //volume ID (in hex)
   int front  =  ( ( (int) stuff[42] ) << 24) & 0xff000000;
   int midLeft = ( ( (int) stuff[41] ) << 16) & 0x00ff0000;
   int midRight =( ( (int) stuff[40] ) << 8 ) & 0x0000ff00;
   int end =     (   (int) stuff[39] )        & 0x000000ff;  
   gPBS.vID = front | midLeft | midRight | end;   

   for (int i = 0; i < 10; i++)
   {
      gPBS.vlb[i] = stuff[i + 43];   //volume label
   }
   for (int i = 0; i < 8; i++)
   {
      gPBS.fst[i] = stuff[i + 54];   //file system type
   }

   printBootSector();
}

void printBootSector()
{   
   printf("Bytes Per Sector: %d\n", gPBS.bps);
   printf("Sectors Per Cluster: %d\n", gPBS.spc);
   printf("Number of FATs: %d\n", gPBS.nFT);
   printf("Number of Reserved Sectors: %d\n", gPBS.nRS);
   printf("Number of Root Entries: %d\n", gPBS.nRE);
   printf("Total Sector Count: %d\n", gPBS.tsc);
   printf("Sectors Per Fat: %d\n", gPBS.spf);
   printf("Sectors Per Track: %d\n", gPBS.spt);
   printf("Number of Heads: %d\n", gPBS.nHD);
   printf("Boot Signature (hex): 0x%0.2x\n", gPBS.bsh);
   printf("Volume ID (hex): 0x%0.6x\n", gPBS.vID);
   printf("Volume Label: %s\n", gPBS.vlb);
   printf("File System Type: %s\n", gPBS.fst);
}


