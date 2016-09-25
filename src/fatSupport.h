
#ifndef _FAT_SUPPORT_H_
#define _FAT_SUPPORT_H_


int read_sector(unsigned int sector_number, unsigned char* buffer);
int write_sector(unsigned int sector_number, unsigned char* buffer);

unsigned int get_fat_entry(unsigned int fat_entry_number, unsigned char* fat);
void set_fat_entry(unsigned int fat_entry_number, unsigned int value, unsigned char* fat);


#endif
