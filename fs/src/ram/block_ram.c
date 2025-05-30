#include "block.h"
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "log.h"

superblock sb;
uchar ramdisk[MAXBLOCKS*BSIZE];

void initialize_superblock(int ncyl, int nsec, uint inode_cnt) {
    Log("initialize with users");
    sb.magic=0x6657;
    sb.size=ncyl*nsec;
    sb.bmapstart=1;
    sb.bmapend=(sb.size / BPB) + 1;
    if(inode_cnt==0){
        sb.imapstart=0;
        sb.imapend=0;
        sb.inode_start=0;
        sb.inode_count=0;
        sb.inode_free=0;
        sb.inode_end=sb.bmapend;
    }
    else{
        sb.imapstart=sb.bmapend+1;
        sb.imapend=sb.bmapend+1;
        sb.inode_start=sb.imapstart+1;
        sb.inode_count=ICOUNT;
        sb.inode_free=ICOUNT;
        sb.inode_end=sb.inode_start+sb.inode_count/4-1;
    }
    sb.data_start=sb.inode_end+1;
    sb.data_block_count=sb.size-sb.data_start;
    sb.data_block_free=sb.data_block_count;
    sb.ncyl = ncyl;
    sb.nsec = nsec;
    sb.current_dir_inum=0;
    for(int i=0;i<sb.data_start;i++){
        write_map(ramdisk,BSIZE*sb.bmapstart,i,1);
    }
    for(int i=sb.data_start;i<sb.size;i++){
        write_map(ramdisk,BSIZE*sb.bmapstart,i,0);
    }
    for(int i=0;i<sb.inode_count;i++){
        write_map(ramdisk,BSIZE*sb.imapstart,i,0);
    }
    // sb.user_num=0;
    // for(int i=0;i<5;i++){
    //     sb.user_id[i]=0;
    // }

}
void initialize_superblock_no_user(int ncyl, int nsec, uint inode_cnt) {
    Log("initialize with no user");
    sb.magic=0x6657;
    sb.size=ncyl*nsec;
    sb.bmapstart=1;
    sb.bmapend=(sb.size / BPB) + 1;
    if(inode_cnt==0){
        sb.imapstart=0;
        sb.imapend=0;
        sb.inode_start=0;
        sb.inode_count=0;
        sb.inode_free=0;
        sb.inode_end=sb.bmapend;
    }
    else{
        sb.imapstart=sb.bmapend+1;
        sb.imapend=sb.bmapend+1;
        sb.inode_start=sb.imapstart+1;
        sb.inode_count=ICOUNT;
        sb.inode_free=ICOUNT;
        sb.inode_end=sb.inode_start+sb.inode_count/4-1;
    }
    sb.data_start=sb.inode_end+1;
    sb.data_block_count=sb.size-sb.data_start;
    sb.data_block_free=sb.data_block_count;
    sb.ncyl = ncyl;
    sb.nsec = nsec;
    sb.current_dir_inum=0;
    for(int i=0;i<sb.data_start;i++){
        write_map(ramdisk,BSIZE*sb.bmapstart,i,1);
    }
    for(int i=sb.data_start;i<sb.size;i++){
        write_map(ramdisk,BSIZE*sb.bmapstart,i,0);
    }
    for(int i=0;i<sb.inode_count;i++){
        write_map(ramdisk,BSIZE*sb.imapstart,i,0);
    }
    sb.user_num=0;
    for(int i=0;i<5;i++){
        sb.user_id[i]=0;
    }

}
bool read_map(uchar* src, uint char_start, uint bit_offset){
    uint char_offset=char_start+bit_offset / 8;
    uint bit_position=bit_offset % 8;
    uchar byte=src[char_offset];
    bool bit=(byte >> (bit_position)) & 1;
    return bit;
}

void write_map(uchar* src, uint char_start, uint bit_offset, bool value){
    uint char_offset=char_start+bit_offset / 8;
    uint bit_position=bit_offset % 8;
    uchar byte=src[char_offset];
    uchar mask=1 << (bit_position);
    if(value){
        byte |= mask;
    }else{
        byte &= ~mask;
    }
    src[char_offset]=byte;
    // Log("write map%d: %d,%d",bit_offset,value,read_map(src,char_start,bit_offset));
}

void zero_block(uint bno) {
    uchar buf[BSIZE];
    memset(buf, 0, BSIZE);
    write_block(bno, buf);
}

uint allocate_block() {
    if(sb.data_block_free == 0){
        Warn("Out of blocks");
        return 0;
    }
    uint base=BSIZE*sb.bmapstart;
    uint bno;
    bool flag=false;
    for(int i = 0; i <sb.size; i++){
        // Log("%d",read_map(ramdisk,base,i));
        if(read_map(ramdisk,base,i)==0){
            bno=i;
            write_map(ramdisk,base,i,1);
            sb.data_block_free--;
            flag=true;
            break;
        }
    }
    update_superblock();
    if(flag == false){
        bno=0;
    }
    Log("Allocate block %d, block_free %d",bno,sb.data_block_free);
    // Log("Data start: %d",sb.data_start);
    return bno;
}

void free_block(uint bno) {
    uint base=BSIZE*sb.bmapstart;
    uint offset=bno;
    if(read_map(ramdisk,base,offset)==0){
        Log("free free block %d",bno);
        return;
    }
    write_map(ramdisk,base,offset,0);
    sb.data_block_free++;
    update_superblock();
    Log("Free block %d",bno);
}

void get_disk_info(int *ncyl, int *nsec) {
    *ncyl=sb.ncyl;
    *nsec=sb.nsec;
}

void read_block(int blockno, uchar *buf) {
    uint start=blockno*BSIZE;
    if(start+BSIZE>MAXBLOCKS*BSIZE){
        Warn("Out of range");
        return;
    }
    memcpy(buf,ramdisk+start,BSIZE);
    Log("Read block %d",blockno);
    return;
}

void write_block(int blockno, uchar *buf) {
    uint start=blockno*BSIZE;
    if(start+BSIZE>MAXBLOCKS*BSIZE){
        Warn("Out of range");
        return;
    }
    memcpy(ramdisk+start,buf,BSIZE);
    Log("Write block %d",blockno);
    return;
}

void update_superblock() {
    uchar buf[BSIZE];
    memset(buf,0,BSIZE);
    memcpy(buf,&sb,sizeof(superblock));
    write_block(0,buf);
    Log("Update superblock");
}

void print_bits(uchar *array, int num_bytes) {
    for (int i = 0; i < num_bytes; i++) {
        printf("%dbyte:", i + 1);
        uchar byte = array[i];
        for (int j = 0; j < 8; j++) {
            unsigned char bit = (byte >> j) & 1;
            printf("%d", bit);
        }
        printf("\n");
    }
}