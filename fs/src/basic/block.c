#include "block.h"
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "log.h"
#include <ctype.h>
#include "tcp_utils.h"

superblock sb;
uchar ramdisk[MAXBLOCKS*BSIZE];
tcp_client client;

int static_ncyl=1024;
int static_nsec=63;

// void change_static_ncyl_nsec(int ncyl, int nsec) {
//     static_ncyl = ncyl;
//     static_nsec = nsec;
// }


void block_client_init(int port) {
    client = client_init("localhost", port);
    int ncyl, nsec;
    get_disk_info(&ncyl, &nsec);
    sb.ncyl = ncyl;
    sb.nsec = nsec;
    static_ncyl = ncyl;
    static_nsec = nsec;
    Log("Block client initialized with ncyl=%d, nsec=%d", ncyl, nsec);
}

void block_client_destroy() {
    client_destroy(client);
    Log("Block client destroyed");
}

void initialize_superblock(int ncyl, int nsec, uint inode_cnt) {
    Log("initialize with users");
    sb.magic=0x6657;
    sb.format=0;
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
    //update bmap
    uchar buf[BSIZE];
    memset(buf, 0, BSIZE);
    // for(int i=0;i<sb.data_start;i++){
    //     // write_map(ramdisk,BSIZE*sb.bmapstart,i,1);
    // }
    // for(int i=sb.data_start;i<sb.size;i++){
    //     write_map(ramdisk,BSIZE*sb.bmapstart,i,0);
    // }
    // for(int i=0;i<sb.inode_count;i++){
    //     write_map(ramdisk,BSIZE*sb.imapstart,i,0);
    // }
    uint bmap_block = sb.bmapstart;
    for(int i=0;i<sb.size;i++){
        uint cur_bmap_block = i / BPB + sb.bmapstart;
        //update bitmap block
        if(cur_bmap_block != bmap_block) {
            write_block(bmap_block, buf);
            bmap_block = cur_bmap_block;
            memset(buf, 0, BSIZE);
        }

        //update bmap
        uint bit_offset = i % BPB;
        if(i < sb.data_start) {
            write_map(buf, 0,bit_offset, 1); // mark as used
        }
        else {
            write_map(buf, 0, bit_offset, 0); // mark as free
        }
    }
    write_block(bmap_block, buf); // write the last block
    //update imap
    if(inode_cnt!=0){
        uint imap_block = sb.imapstart;
        memset(buf, 0, BSIZE);
        write_block(imap_block, buf);
    }

    // sb.user_num=0;
    // for(int i=0;i<5;i++){
    //     sb.user_id[i]=0;
    // }

}
void initialize_superblock_no_user(int ncyl, int nsec, uint inode_cnt) {
    Log("initialize with no user");
    sb.magic=0x6657;
    sb.format=0;
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
    // for(int i=0;i<sb.data_start;i++){
    //     write_map(ramdisk,BSIZE*sb.bmapstart,i,1);
    // }
    // for(int i=sb.data_start;i<sb.size;i++){
    //     write_map(ramdisk,BSIZE*sb.bmapstart,i,0);
    // }
    // for(int i=0;i<sb.inode_count;i++){
    //     write_map(ramdisk,BSIZE*sb.imapstart,i,0);
    // }
    //update bmap
    uchar buf[BSIZE];
    memset(buf, 0, BSIZE);
    uint bmap_block = sb.bmapstart;
    for(int i=0;i<sb.size;i++){
        uint cur_bmap_block = i / BPB + sb.bmapstart;
        //update bitmap block
        if(cur_bmap_block != bmap_block) {
            write_block(bmap_block, buf);
            bmap_block = cur_bmap_block;
            memset(buf, 0, BSIZE);
        }

        //update bmap
        uint bit_offset = i % BPB;
        if(i < sb.data_start) {
            write_map(buf, 0,bit_offset, 1); // mark as used
        }
        else {
            write_map(buf, 0, bit_offset, 0); // mark as free
        }
    }
    write_block(bmap_block, buf); // write the last block
    //update imap
    uint imap_block = sb.imapstart;
    memset(buf, 0, BSIZE);
    write_block(imap_block, buf);

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
    // uint base=BSIZE*sb.bmapstart;
    uint bno;
    bool flag=false;

    uchar buf[BSIZE];
    read_block(sb.bmapstart, buf);
    uint bit_block = sb.bmapstart;
    for(int i = 0; i <sb.size; i++){
        uint cur_bit_block = i / BPB + sb.bmapstart;
        if(cur_bit_block != bit_block) {
            read_block(cur_bit_block, buf);
            bit_block = cur_bit_block;
        }
        // Log("%d",read_map(ramdisk,base,i));
        // if(read_map(ramdisk,base,i)==0){
        //     bno=i;
        //     write_map(ramdisk,base,i,1);
        //     sb.data_block_free--;
        //     flag=true;
        //     break;
        // }
        uint bit_offset = i % BPB;
        if(read_map(buf, 0, bit_offset) ==0){
            bno=i;
            write_map(buf, 0, bit_offset, 1);

            flag=true;
            sb.data_block_free--;
            write_block(bit_block, buf);
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
    // uint base=BSIZE*sb.bmapstart;
    uint offset=bno;
    // if(read_map(ramdisk,base,offset)==0){
    //     Log("free free block %d",bno);
    //     return;
    // }
    // write_map(ramdisk,base,offset,0);
    uchar buf[BSIZE];
    uint bit_block = sb.bmapstart + offset / BPB;
    uint bit_offset = offset % BPB;
    read_block(bit_block, buf);
    if(read_map(buf, 0, bit_offset) == 0){
        Log("Free free block %d", bno);
        return;
    }
    write_map(buf, 0, bit_offset, 0);
    write_block(bit_block, buf);
    sb.data_block_free++;
    update_superblock();
    Log("Free block %d",bno);
}

void get_disk_info(int *ncyl, int *nsec) {
    char send[] = "I";
    char buf[4096];
    client_send(client, send, sizeof(send));
    client_recv(client, buf, 4096);
    char *cyl= strtok(buf, " \r\n");
    char *sec = strtok(NULL, " \r\n");
    *ncyl = atoi(cyl);
    *nsec = atoi(sec);
    Log("Disk info: ncyl=%d, nsec=%d", *ncyl, *nsec);
}

void read_block(int blockno, uchar *buf) {
    uint start=blockno*BSIZE;
    if(start+BSIZE>MAXBLOCKS*BSIZE){
        Warn("Out of range");
        return;
    }
    // memcpy(buf,ramdisk+start,BSIZE);

    uint cyl= blockno / static_nsec;
    uint sec= blockno % static_nsec;
    char send[64];
    snprintf(send, sizeof(send), "R %d %d", cyl, sec);
    client_send(client, send, strlen(send) + 1);
    char disk_buf[4096]={};
    client_recv(client, disk_buf, 4096);
    // debug_print_bytes(disk_buf, 25);
    memcpy(buf, disk_buf+4, BSIZE);
    // Log("Read block %d",blockno);
    Log("Read block %d at cyl %d sec %d", blockno, cyl, sec);
    return;
}

void write_block(int blockno, uchar *buf) {
    uint start=blockno*BSIZE;
    if(start+BSIZE>MAXBLOCKS*BSIZE){
        Warn("Out of range");
        return;
    }
    // memcpy(ramdisk+start,buf,BSIZE);
    // Log("Write block %d",blockno);
    uint cyl= blockno / static_nsec;
    uint sec= blockno % static_nsec;
    char send[4096];
    send[0] = '\0';

    int offset = 0;
    int written = snprintf(send, sizeof(send), "W %d %d %d ", cyl, sec, BSIZE);
    offset += written;

    memcpy(send + offset, buf, BSIZE);


    client_send(client, send, offset + BSIZE);
    char disk_buf[4096];
    client_recv(client, disk_buf, 4096);
    Log("Write block %d at cyl %d sec %d", blockno, cyl, sec);
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

void debug_print_bytes(char *data, int n) {
    if (data == NULL) {
        Log("Data is NULL!\n");
        return;
    }

    // 打印 n 的值（如果 n 大于实际数据长度，只打印到数据末尾）
    Log("Printing first %d bytes of data:\n", n);

    // 计算实际要打印的字节数（不能超过数据长度）
    int bytes_to_print = n;

    // 逐字节打印数据
    for (int i = 0; i < bytes_to_print; i++) {
        // 如果是可打印字符，直接打印字符本身
        if (isprint((unsigned char)data[i])) {
            Log("%c ", data[i]);
        } else {
            // 否则，打印十六进制值并标注
            Log("[0x%02x] ", (unsigned char)data[i]);
        }

    }
    // 如果最后不是刚好 16 个字节换行，确保最后有一个换行
    if (bytes_to_print % 16 != 0) {
        Log("\n");
    }
}