#ifndef __BLOCK_H__
#define __BLOCK_H__

#include "common.h"
#define ICOUNT 60;
typedef struct {
    uint magic;      // Magic number, used to identify the file system
    int size;       // Size in blocks
    uint bmapstart;  // Block number of first free map block
    uint bmapend;    // Block number of last free map block
    uint imapstart;  // Block number of first inode map block
    uint imapend;    // Block number of last inode map block
    uint data_block_count; // Number of data blocks
    uint inode_count; // Number of inodes
    uint inode_start; // Block number of first inode block
    uint inode_end;   // Block number of last inode block
    uint data_start;  // Block number of first data block
    uint data_block_free; // Number of available data blocks
    uint inode_free;  // Number of available inodes
    int ncyl;
    int nsec;
    uint user_num;
    int user_id[5];
    uint current_dir_inum;
    // ...
    // ...
    // Other fields can be added as needed
} superblock;

// sb is defined in block.c
// extern uchar ramdisk[MAXBLOCKS*BSIZE];
extern superblock sb;
void initialize_superblock(int ncyl, int nsec, uint inode_cnt);
void initialize_superblock_no_user(int ncyl, int nsec, uint inode_cnt);
bool read_map(uchar* src, uint char_start, uint bit_offset);
void write_map(uchar* src, uint char_start, uint bit_offset, bool value);
void zero_block(uint bno);
uint allocate_block();
void free_block(uint bno);

void get_disk_info(int *ncyl, int *nsec);
void read_block(int blockno, uchar *buf);
void write_block(int blockno, uchar *buf);

void update_superblock();


#endif