#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "fat.h"

typedef struct PBS
{

   int bps;   //bytes per sector
   int spc;   //sectors per cluster
   int nFT;   //number of FATs
   int nRS;   //number of reserved sectors
   int nRE;   //number of root entries
   int tsc;   //total sector count
   int spf;   //sectors per FAT
   int spt;   //sectors per track
   int nHD;   //number of heads
   int bsh;   //boot signature (in hex)
   int vID;   //volume ID (in hex)
   unsigned char vlb[11];   //volume label
   unsigned char fst[8];   //file system type

   unsigned char* contents;
}PBS;

void readBootSector();
void printBootSector();
