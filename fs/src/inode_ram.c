#include "inode.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "block.h"
#include "log.h"

inode *iget(uint inum) {
    if(inum>=sb.inode_count){
        Error("iget: inum out of range");
        return NULL;
    }
    uchar imap_buf[BSIZE];
    read_block(sb.imapstart,imap_buf);
    if(!read_map(imap_buf,0,inum)){
        Error("iget: inode not allocated");
        return NULL;
    }
    dinode ip[4];
    read_block(sb.inode_start+inum/4,(uchar*)ip);
    inode *iptr=malloc(sizeof(inode));
    if(iptr==NULL){
        Error("iget: malloc failed");
        return NULL;
    }
    iptr->inum=inum;
    iptr->type=ip[inum%4].type;
    iptr->owner=ip[inum%4].owner;
    iptr->mode=ip[inum%4].mode;
    iptr->group=ip[inum%4].group;
    iptr->creation_time=ip[inum%4].creation_time;
    iptr->modify_time=ip[inum%4].modify_time;
    iptr->size=ip[inum%4].size;
    iptr->blocks=ip[inum%4].blocks;
    iptr->reference_count=1;
    for(int i=0;i<NDIRECT+2;i++){
        iptr->addrs[i]=ip[inum%4].addrs[i];
    }
    memcpy(iptr->name,ip[inum%4].name,MAXNAME);
    return iptr;
}

void iput(inode *ip) { 
    if(ip==NULL){
        return;
    }
    if(ip->reference_count>1){
        ip->reference_count--;
        return;
    }
    free(ip); 
}

inode *ialloc(short type) {
    uint available_inode=sb.inode_free;
    if(available_inode==0){
        Error("ialloc: no inodes");
        return NULL;
    }
    sb.inode_free--;
    uchar imap_buf[BSIZE];
    read_block(sb.imapstart,imap_buf);
    // print_bits(imap_buf,1);
    inode* ip=NULL;
    for(int i=0;i<sb.inode_count;i++){
        if(read_map(imap_buf,0,i)==0){
            Log("free inode%d",i);
            write_map(imap_buf,0,i,true);
            write_block(sb.imapstart,imap_buf);
            ip=malloc(sizeof(inode));
            if(ip==NULL){
                Error("ialloc: malloc failed");
                return NULL;
            }
            ip->inum=i;
            ip->type=type;
            ip->owner=0;
            ip->mode=0;
            ip->group=0;
            ip->creation_time=time(NULL);
            ip->modify_time=time(NULL);
            ip->size=0;
            ip->blocks=0;
            ip->reference_count=1;
            for(int j=0;j<NDIRECT+2;j++){
                ip->addrs[j]=0;
            }
            Log("ialloc: inode %d allocated", i);
            iupdate(ip);
            break;
        }
    }
    update_superblock();
    return ip;
}



void iupdate(inode *ip) {
    uint inum=ip->inum;
    if(ip==NULL){
        Error("iupdate: inode is NULL");
        return;
    }
    if(inum>=sb.inode_count){
        Error("iupdate: inum out of range");
        return;
    }
    dinode din[4];
    read_block(sb.inode_start+inum/4,(uchar*)din);
    din[inum%4].type=ip->type;
    din[inum%4].owner=ip->owner;
    din[inum%4].mode=ip->mode;
    din[inum%4].group=ip->group;
    din[inum%4].creation_time=ip->creation_time;
    din[inum%4].modify_time=ip->modify_time;
    din[inum%4].size=ip->size;
    din[inum%4].blocks=ip->blocks;
    memcpy(din[inum%4].name,ip->name,MAXNAME);
    for(int i=0;i<NDIRECT+2;i++){
        din[inum%4].addrs[i]=ip->addrs[i];
    }
    write_block(sb.inode_start+inum/4,(uchar*)din);
    Log("iupdate: inode %d updated", inum);
    return;

}

int readi(inode *ip, uchar *dst, uint off, uint n) {
    Log("readi off: %d, n:%d",off,n);
    if(off==0 && n==0){
        dst[0]='\0';
        return 0;
    }
    if(ip==NULL){
        dst[0]='\0';
        Error("readi: inode is NULL");
        return 0;
    }
    if(off >= ip->size){
        dst[0]='\0';
        Error("readi: offset out of range");
        return 0;
    }
    if(off+n>ip->size){
        return readi(ip,dst,off,ip->size-off);
    }

    uint block_base = off / BSIZE;
    uint block_offset = off % BSIZE;
    bool fetch_indirect = (block_base >= NDIRECT) || (off + n > NDIRECT * BSIZE);
    if(!fetch_indirect){
        uchar *tmp=dst;
        uint block_access = block_base;
        uint n_left = n;
        while(true){
            uchar buf[BSIZE];
            read_block(ip->addrs[block_access], buf);
            if(block_access==block_base){
                if(n_left+block_offset > BSIZE){
                    memcpy(tmp, buf+block_offset, BSIZE-block_offset);
                    tmp += BSIZE-block_offset;
                    n_left -= BSIZE-block_offset;
                    block_access++;
                    continue;
                } else {
                    memcpy(tmp, buf+block_offset, n_left);
                    n_left=0;
                    break;
                }
                
            }
            if(n_left > BSIZE){
                memcpy(tmp, buf, BSIZE);
                tmp += BSIZE;
                n_left -= BSIZE;
                block_access++;
            } else {
                memcpy(tmp, buf, n_left);
                n_left=0;
                break;
            }
        }
        return n;
    }
    // fetch indirect is true

    uint n_left = n;
    uchar *tmp=dst;
    // uint size_left = ip->size - off;

    for(int i=block_base;i<NDIRECT;i++){
        uchar buf[BSIZE];
        read_block(ip->addrs[i], buf);
        if(i==block_base){
            memcpy(tmp, buf+block_offset, BSIZE-block_offset);
            tmp += BSIZE-block_offset;
            n_left -= BSIZE-block_offset;
            continue;
        }
        memcpy(tmp, buf, BSIZE);
        tmp += BSIZE;
        n_left -= BSIZE;
    }
    for( int i=NDIRECT;i<NDIRECT+2;i++){
        if(off>127*BSIZE+NDIRECT*BSIZE && i==NDIRECT){
            continue;
        }
        indirect_block* indirect_buf=malloc(sizeof(indirect_block));
        read_block(ip->addrs[i], (uchar*)indirect_buf);
        Log("available blocks%d",indirect_buf->available_blocks);
        for(int j=0;j<indirect_buf->available_blocks;j++){
            uint cur_size=NDIRECT*BSIZE+j*BSIZE+(i-NDIRECT)*127*BSIZE;
            if(off>cur_size+BSIZE){
                continue;
            }
            uchar buf[BSIZE];
            read_block(indirect_buf->datablock[j], buf);
            if(block_base<NDIRECT){
                if(n_left > BSIZE){
                    memcpy(tmp, buf, BSIZE);
                    tmp += BSIZE;
                    n_left -= BSIZE;
                } else {
                    memcpy(tmp, buf, n_left);
                    n_left = 0;
                    break;
                }
                continue;
            }else{
                if(block_base==NDIRECT+(i-NDIRECT)*127+j){
                    if(n_left+block_offset > BSIZE){
                        memcpy(tmp, buf+block_offset, BSIZE-block_offset);
                        tmp += BSIZE-block_offset;
                        n_left -= BSIZE-block_offset;
                        continue;
                    } else {
                        memcpy(tmp, buf+block_offset, n_left);
                        n_left=0;
                        break;
                    }
                }
                if(n_left > BSIZE){
                    memcpy(tmp, buf, BSIZE);
                    tmp += BSIZE;
                    n_left -= BSIZE;
                } else {
                    memcpy(tmp, buf, n_left);
                    n_left = 0;
                    break;
                }
            }
            
        }
        free(indirect_buf);
        if(n_left == 0){
            break;
        }
    }
    return n;
}

int writei(inode *ip, uchar *src, uint off, uint n) {

    uint size_update=max(off+n,ip->size);
    ip->size=size_update;//update size
    uint block_base=off/BSIZE;
    uint block_offset=off%BSIZE;
    uint block_to_use=(off+n)/BSIZE+1;
    Log("writei off: %d, n: %d, block_to_use: %d",off,n,block_to_use);
    if(block_to_use>NDIRECT+2*127){
        Error("out of bound!");
        return 0;
    }
    //check the block and allocate first
    for(int i=0;i<min(block_to_use,NDIRECT);i++){
        if(ip->addrs[i]==0){
            ip->addrs[i]=allocate_block();
        }
    }
    for(int i=NDIRECT;i<NDIRECT+2;i++){
        if(block_to_use<NDIRECT+(i-NDIRECT)*127){
            break;
        }
        if(ip->addrs[i]==0){
            ip->addrs[i]=allocate_block();
            indirect_block* in_block=malloc(sizeof(indirect_block));
            in_block->available_blocks=0;
            for(int j=0;j<127;j++){
                in_block->datablock[j]=0;
            }
            write_block(ip->addrs[i],(uchar*)in_block);
            free(in_block);

        }
        if(block_to_use<NDIRECT+127){
            uint addtional_block=block_to_use-NDIRECT;
            indirect_block* in_block=malloc(sizeof(indirect_block));
            read_block(ip->addrs[i],(uchar*)in_block);
            uint current_blocks=in_block->available_blocks;
            in_block->available_blocks=addtional_block;
            // for(int j=0;j<127;j++){
            //     in_block->datablock[j]=0;
            // }
            for(int j=current_blocks;j<addtional_block;j++){
                in_block->datablock[j]=allocate_block();
            }
            write_block(ip->addrs[i],(uchar*)in_block);
            free(in_block);
            break;
        }
        indirect_block* in_block=malloc(sizeof(indirect_block));
        read_block(ip->addrs[i],(uchar*)in_block);
        in_block->available_blocks=127;
        for(int j=0;j<127;j++){
            in_block->datablock[j]=allocate_block();
        }
        free(in_block);
        break;
    }
    //write the new data
    Log("finish allocate");
    uchar* tmp=src;
    uint n_left=n;
    for(int i=block_base;i<NDIRECT;i++){
        uint block_addr=ip->addrs[i];
        uchar buf[BSIZE];
        if(i==block_base){
            uint cpy_length=min(n_left,BSIZE-block_offset);
            read_block(block_addr,buf);
            memcpy(buf+block_offset,tmp,cpy_length);
            write_block(block_addr,buf);
            tmp+=cpy_length;
            n_left-=cpy_length;
            if(n_left==0){
                break;
            }
        }
        else{
            uint cpy_length=min(n_left,BSIZE);
            read_block(block_addr,buf);
            memcpy(buf,tmp,cpy_length);
            write_block(block_addr,buf);
            tmp+=cpy_length;
            n_left-=cpy_length;
            if(n_left==0){
                break;
            }
        }
    }
    if(off+n>NDIRECT*BSIZE){
        for(int i=NDIRECT;i<NDIRECT+2;i++){
            if(n_left==0){
                break;
            }
            uint ind_addr=ip->addrs[i];
            Log("ind_addr: %d",ind_addr);
            if(block_base>NDIRECT+(i+1-NDIRECT)*127){\
                continue;
            }
            indirect_block *id_block=malloc(sizeof(indirect_block));
            read_block(ind_addr,(uchar*)id_block);
            int tmpp=block_base-NDIRECT-(i-NDIRECT)*127;
            int ind_start=max(tmpp,0);
            Log("available blocks: %d",id_block->available_blocks);
            for(int j=ind_start;j<id_block->available_blocks;j++){
                uint block_addr=id_block->datablock[j];
                uchar buf[BSIZE];
                if(j==tmpp){
                    uint cpy_length=min(n_left,BSIZE-block_offset);
                    read_block(block_addr,buf);
                    memcpy(buf+block_offset,tmp,cpy_length);
                    write_block(block_addr,buf);
                    tmp+=cpy_length;
                    n_left-=cpy_length;
                    if(n_left==0){
                        break;
                    }
                }
                else{
                    uint cpy_length=min(n_left,BSIZE);
                    read_block(block_addr,buf);
                    memcpy(buf,tmp,cpy_length);
                    write_block(block_addr,buf);
                    tmp+=cpy_length;
                    n_left-=cpy_length;
                    if(n_left==0){
                        break;
                    }
                }
            }
            free(id_block);
        }
    }
    ip->modify_time=time(NULL);
    ip->blocks=max(ip->blocks,ip->size/BSIZE+1);
    iupdate(ip);
    return n;
}

void changename(inode *ip, char* name){
    strcpy(ip->name, name);
    iupdate(ip);
    return;
}

void changesize(inode* ip, uint size){
    ip->size=size;
    uint block_update=size/BSIZE +1;
    for(int i=block_update;i<min(ip->blocks,NDIRECT);i++){
        free_block(ip->addrs[i]);
        ip->addrs[i]=0;
    }
    if(ip->blocks>NDIRECT){
        for(int i=NDIRECT;i<NDIRECT+2;i++){
            if(ip->blocks<NDIRECT+(i-NDIRECT)*127){
                break;
            }
            if(block_update>NDIRECT+(i+1-NDIRECT)*127){
                continue;
            }
            uint in_addr=ip->addrs[i];
            indirect_block *in_block=malloc(sizeof(indirect_block));
            read_block(in_addr,(uchar*)in_block);
            int block_update_int=block_update;
            for(int j=min(block_update_int-NDIRECT-(i-NDIRECT)*127,0);j<in_block->available_blocks;j++){
                free_block(in_block->datablock[j]);
                in_block->datablock[j]=0;
            }
            in_block->available_blocks=min(block_update_int-NDIRECT-(i-NDIRECT)*127,0);
            if(in_block->available_blocks==0){
                free_block(ip->addrs[i]);
                ip->addrs[i]=0;
            }
            free(in_block);
        }
    }
    ip->blocks=block_update;
    iupdate(ip);
    return;
}

void deletei(inode *ip){
    if(ip->size!=0){
        changesize(ip,0);
        free_block(ip->addrs[0]);
    }
    uchar imp_buf[BSIZE];
    read_block(sb.imapstart,imp_buf);
    write_map(imp_buf,0,ip->inum,0);
    iput(ip);
    return;
}