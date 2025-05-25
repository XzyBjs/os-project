#ifndef __INODE_H__
#define __INODE_H__

#include "common.h"

#define NDIRECT 18  // Direct blocks, you can change this value

#define MAXFILEB (NDIRECT + APB + APB * APB)

enum {
    T_DIR = 1,   // Directory
    T_FILE = 2,  // File
};


// You can change the size of MAXNAME
#define MAXNAME 14

// You should add more fields
// the size of a dinode must divide BSIZE
typedef struct {
    ushort type;              // File type
    uint link_count;          // Link count
    int owner;               // owner id
    short mode;               // mode
    short group;              // group id
    char name[MAXNAME];      // file name
    uint creation_time;       // creation time
    uint modify_time;          // modification time
    uint size;                // Size in bytes
    uint blocks;              // Number of blocks, may be larger than size
    uint addrs[NDIRECT + 2];  // Data block addresses,
                                // the last two are indirect blocks
    
    // ...
    // ...
    // Other fields can be added as needed
} dinode;

// inode in memory
// more useful fields can be added, e.g. reference count
typedef struct {
    uint inum;
    ushort type;
    uint owner;
    short mode;
    short group;
    uint creation_time; 
    uint modify_time;
    char name[MAXNAME];
    uint size;
    uint blocks;
    uint reference_count;
    uint addrs[NDIRECT + 2];
    // ...
    // ...
    // Other fields can be added as needed
} inode;


typedef struct{
    uint available_blocks;
    uint datablock[127];
} indirect_block;

// Get an inode by number (returns allocated inode or NULL)
// Don't forget to use iput()
inode *iget(uint inum);

// Free an inode (or decrement reference count)
void iput(inode *ip);

// Allocate a new inode of specified type (returns allocated inode or NULL)
// Don't forget to use iput()
inode *ialloc(short type);

// Update disk inode with memory inode contents
void iupdate(inode *ip);

// Read from an inode (returns bytes read or -1 on error)
int readi(inode *ip, uchar *dst, uint off, uint n);

// Write to an inode (returns bytes written or -1 on error)
int writei(inode *ip, uchar *src, uint off, uint n);


void deletei(inode *ip);

void changename(inode *ip, char* name);

void changesize(inode *ip, uint size);
#endif
