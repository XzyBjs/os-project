#include "fs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"
#include "log.h"


void int_to_char(int num, char *result) {
    int is_negative = 0;
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    int digits = 0;
    int temp = num;
    while (temp > 0) {
        digits++;
        temp /= 10;
    }

    if (digits == 0) {
        digits = 1;
    }

    if (is_negative) {
        digits++;
    }


    int i;
    for (i = digits - 1; i >= 0; i--) {
        if (is_negative && i == digits - 1) {
            result[i] = '-';
        } else {
            int digit = num % 10;
            result[i] = digit + '0';  
            num /= 10;
        }
    }

    result[digits] = '\0';
}

void sbinit() {
    uchar buf[BSIZE];
    read_block(0, buf);
    memcpy(&sb, buf, sizeof(sb));
}

int cmd_f(int ncyl, int nsec) {
    sbinit();
    if(sb.magic!=0x6657){
        initialize_superblock_no_user(ncyl,nsec,60);
    }
    else{
        initialize_superblock(ncyl,nsec,60);
    }

    if(iget(0)!=NULL){
        inode* root=iget(0);
        deletei(root);
    }
    inode* root=ialloc(T_DIR);
    char root_name[]="";
    strcpy(root->name,root_name);
    root->mode=-1;

    directory* root_dir=malloc(sizeof(directory));
    root_dir->num=2;
    root_dir->entries[0].type=T_DIR;
    root_dir->entries[0].size=0;
    root_dir->entries[0].user=0;
    root_dir->entries[0].mode=0;
    char back_name[]="..";
    strcpy(root_dir->entries[0].name,back_name);

    char home_name[]="users";
    root_dir->entries[0].inum=root->inum;
    root_dir->entries[1].type=T_DIR;
    root_dir->entries[1].size=0;
    strcpy(root_dir->entries[1].name,home_name);
    if(iget(1)!=NULL){
        inode* home=iget(1);
        deletei(home);
    }
    inode* home=ialloc(T_DIR);
    root_dir->entries[1].inum=home->inum;//home dir
    root_dir->entries[1].update_time=home->modify_time;
    root_dir->entries[1].user=0;
    root_dir->entries[1].mode=-1;
    writei(root,(uchar*)root_dir,0,sizeof(directory));

    directory* home_dir=malloc(sizeof(directory));

    home->mode=-1;
    home->group=0;
    strcpy(home->name,home_name);

    home_dir->num=sb.user_num+1;
    home_dir->entries[0].type=T_DIR;
    home_dir->entries[0].size=0;
    home_dir->entries[0].inum=root->inum;
    home_dir->entries[0].user=0;
    home_dir->entries[0].mode=0;
    strcpy(home_dir->entries[0].name,back_name);//set ..

    for(int i=1;i<sb.user_num+1;i++){
        home_dir->entries[i].type=T_DIR;
        home_dir->entries[i].size=0;
        inode* user=ialloc(T_DIR);
        home_dir->entries[i].inum=user->inum;
        char user_name[MAXNAME];
        int_to_char(sb.user_id[i-1],user_name);
        strcpy(home_dir->entries[i].name,user_name);

        directory* user_dir=malloc(sizeof(directory));
        user_dir->num=1;
        user_dir->entries[0].size=0;
        user_dir->entries[0].type=T_DIR;
        user_dir->entries[0].inum=home->inum;
        user_dir->entries[0].mode=0;
        strcpy(user_dir->entries[0].name,back_name);//set ..

        writei(user,(uchar*)user_dir,0,sizeof(directory));

        user->owner=sb.user_id[i];
        user->group=0;
        user->mode=-1;
        

        iupdate(user);
        free(user_dir);
        iput(user);
    }
    writei(home,(uchar*)home_dir,0,sizeof(directory));

    sb.current_dir_inum=root->inum;
    

    iupdate(home);
    iupdate(root);
    iput(home);
    iput(root);
    free(home_dir);
    free(root_dir);
    update_superblock();
    Log("cmd_format");
    return E_SUCCESS;
}

int cmd_mk(char *name, short mode) {
    inode* current_dir=iget(sb.current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",sb.current_dir_inum);
        return E_ERROR;
    }

    directory* dir=malloc(sizeof(directory));
    readi(current_dir,(uchar*)dir,0,sizeof(directory));
    //check existence
    for(int i=0;i<dir->num;i++){
        // Log("follow name: %s, size: %d",dir->entries[i].name,sizeof(dir->entries[i].name));
        if(strcmp(name,dir->entries[i].name)==0){
            Error("File of same name exist: %s",name);
            iput(current_dir);
            free(dir);
            return E_ERROR;
        }
    }

    //create file
    inode* new_file=ialloc(T_FILE);
    new_file->mode=mode;
    strcpy(new_file->name,name);

    //update dir
    dir->entries[dir->num].inum=new_file->inum;
    dir->entries[dir->num].mode=mode;
    dir->entries[dir->num].size=0;
    dir->entries[dir->num].type=T_FILE;
    dir->entries[dir->num].update_time=new_file->modify_time;
    strcpy(dir->entries[dir->num].name,name);
    dir->num++;

    //update current dir inode
    writei(current_dir,(uchar*)dir,0,sizeof(directory));


    Log("File made: %s",name);
    iupdate(current_dir);
    iupdate(new_file);
    iput(current_dir);
    iput(new_file);
    free(dir);
    return E_SUCCESS;
}

int cmd_mkdir(char *name, short mode) {
    inode* current_dir=iget(sb.current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",sb.current_dir_inum);
        return E_ERROR;
    }

    directory *dir=malloc(sizeof(directory));
    readi(current_dir,(uchar*)dir,0,sizeof(directory));
    //check existence
    for(int i=0;i<dir->num;i++){
        // Log("follow name: %s, size: %d",dir->entries[i].name,sizeof(dir->entries[i].name));
        if(strcmp(name,dir->entries[i].name)==0){
            Error("Dir of same name exist: %s",name);
            iput(current_dir);
            free(dir);
            return E_ERROR;
        }
    }

    //create dir
    inode* new_dir_i=ialloc(T_DIR);
    new_dir_i->mode=mode;
    strcpy(new_dir_i->name,name);

    directory *new_dir=malloc(sizeof(directory));
    new_dir->num=1;
    new_dir->entries[0].inum=current_dir->inum;
    new_dir->entries[0].mode=0;
    new_dir->entries[0].type=T_DIR;
    new_dir->entries[0].size=0;
    strcpy(new_dir->entries[0].name,".."); //set ..
    writei(new_dir_i,(uchar*)new_dir,0,sizeof(directory));

    //update dir
    dir->entries[dir->num].inum=new_dir_i->inum;
    dir->entries[dir->num].mode=mode;
    dir->entries[dir->num].size=0;
    dir->entries[dir->num].type=T_DIR;
    dir->entries[dir->num].update_time=new_dir_i->modify_time;
    strcpy(dir->entries[dir->num].name,name);
    dir->num++;

    //update current dir inode
    writei(current_dir,(uchar*)dir,0,sizeof(directory));

    Log("Dir made: %s",name);
    iupdate(current_dir);
    iupdate(new_dir_i);
    iput(new_dir_i);
    iput(current_dir);
    free(dir);
    free(new_dir);
    return E_SUCCESS;
}

int cmd_rm(char *name) {
    inode* current_dir=iget(sb.current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",sb.current_dir_inum);
        return E_ERROR;
    }

    directory *dir=malloc(sizeof(directory));
    readi(current_dir,(uchar*)dir,0,sizeof(directory));

    //check existence
    int target_num=0;
    bool exist=false;
    for(int i=1;i<dir->num;i++){
        // Log("follow name: %s, size: %d",dir->entries[i].name,sizeof(dir->entries[i].name));
        if(strcmp(name,dir->entries[i].name)==0 && dir->entries[i].type==T_FILE){
            Log("File of same name exist: %s",name);
            target_num=i;
            exist=true;
            break;
        }
    }

    if(exist==false){
        Error("No such file: %s",name);
        iput(current_dir);
        free(dir);
        return E_ERROR;
    }


    //delete file
    inode* target=iget(dir->entries[target_num].inum);
    deletei(target);

    //update current dir
    for(int i=target_num+1;i<dir->num;i++){
        dir->entries[i-1].inum=dir->entries[i].inum;
        dir->entries[i-1].mode=dir->entries[i].mode;
        dir->entries[i-1].size=dir->entries[i].size;
        dir->entries[i-1].type=dir->entries[i].type;
        dir->entries[i-1].update_time=dir->entries[i-1].update_time;
        // dir->entries[i-1].user=dir->entries[i].user;
        strcpy(dir->entries[i-1].name,dir->entries[i].name);
    }
    dir->num--;

    //write back
    writei(current_dir,(uchar*)dir,0,sizeof(directory));

    iupdate(current_dir);
    iput(current_dir);
    free(dir);
    return E_SUCCESS;
}

int cmd_cd(char *name) {
    char* tmp=malloc(sizeof(char)*100);
    strcpy(tmp,name);
    inode* current_dir=NULL;
    directory* dir=malloc(sizeof(directory));
    char* token=NULL;
    bool absolute=(tmp[0]=='/')?1:0;
    if(absolute){
        //get root
        current_dir=iget(0);
        
        token=strtok(tmp+1,"/");
        // Log("cur_cd: %s",token);
    }
    else{
        current_dir=iget(sb.current_dir_inum);

        token=strtok(tmp,"/");
    }



    while(token != NULL){
        readi(current_dir,(uchar*)dir,0,sizeof(directory));
        int target_num;
        bool exist=false;
        //check existence
        for(int i=0;i<dir->num;i++){
            // Log("follow name: %s, size: %d",dir->entries[i].name,sizeof(dir->entries[i].name));
            if(strcmp(token,dir->entries[i].name)==0 && dir->entries[i].type==T_DIR){
                // Log("Dir of same name exist: %s",name);
                target_num=i;
                exist=true;
                break;
            }   
        }

        if(exist==false){
            Error("Wrong dir: %s",name);
            iput(current_dir);
            free(dir);
            return E_ERROR;
        }

        uint next_inum=dir->entries[target_num].inum;

        iput(current_dir);
        current_dir=iget(next_inum);

        token=strtok(NULL,"/");
    }
    

    //update sb
    sb.current_dir_inum=current_dir->inum;
    update_superblock();
    iput(current_dir);
    free(dir);
    free(tmp);
    return E_SUCCESS;
}

int cmd_rmdir(char *name) {
    inode* current_dir=iget(sb.current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",sb.current_dir_inum);
        return E_ERROR;
    }
    directory* dir=malloc(sizeof(directory));
    readi(current_dir,(uchar*)dir,0,sizeof(directory));

    //check existence
    int target_num=0;
    bool exist=false;
    for(int i=1;i<dir->num;i++){
        // Log("follow name: %s, size: %d",dir->entries[i].name,sizeof(dir->entries[i].name));
        if(strcmp(name,dir->entries[i].name)==0 && dir->entries[i].type==T_DIR){
            Log("Dir of same name exist: %s",name);
            target_num=i;
            exist=true;
            break;
        }
    }

    if(exist==false){
        Error("No such dir: %s",name);
        iput(current_dir);
        free(dir);
        return E_ERROR;
    }

    //chech the target
    inode* target=iget(dir->entries[target_num].inum);
    directory* target_dir=malloc(sizeof(directory));
    readi(target,(uchar*)target_dir,0,sizeof(directory));

    if(target_dir->num>1){
        Error("Dir is not empty: %s",name);
        free(target_dir);
        iput(target);
        iput(current_dir);
        free(dir);
        return E_ERROR;
    }
    
    //delete the dir
    deletei(target);

    //update current dir
    for(int i=target_num+1;i<dir->num;i++){
        dir->entries[i-1].inum=dir->entries[i].inum;
        dir->entries[i-1].mode=dir->entries[i].mode;
        dir->entries[i-1].size=dir->entries[i].size;
        dir->entries[i-1].type=dir->entries[i].type;
        dir->entries[i-1].update_time=dir->entries[i-1].update_time;
        // dir->entries[i-1].user=dir->entries[i].user;
        strcpy(dir->entries[i-1].name,dir->entries[i].name);
    }
    dir->num--;

    //write back
    writei(current_dir,(uchar*)dir,0,sizeof(directory));

    iupdate(current_dir);
    iput(current_dir);
    free(dir);
    free(target_dir);
    return E_SUCCESS;
}
int cmd_ls(entry **entries, int *n) {
    inode* current_dir = iget(sb.current_dir_inum);
    directory* dir=malloc(sizeof(directory));
    readi(current_dir,(uchar*)dir,0,sizeof(directory));

    *n=dir->num-1;
    Log("dir num: %d",dir->num);
    *entries=malloc(sizeof(entry)*(dir->num-1));
    for(int i=1;i<dir->num;i++){
        (*entries)[i-1].inum=dir->entries[i].inum;
        (*entries)[i-1].mode=dir->entries[i].mode;
        (*entries)[i-1].size=dir->entries[i].size;
        (*entries)[i-1].type=dir->entries[i].type;
        (*entries)[i-1].update_time=dir->entries[i].update_time;
        (*entries)[i-1].user=dir->entries[i].user;
        strcpy((*entries)[i-1].name,dir->entries[i].name);
    }


    iput(current_dir);
    free(dir);
    return E_SUCCESS;
}

int cmd_cat(char *name, uchar **buf, uint *len) {
    inode* current_dir=iget(sb.current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",sb.current_dir_inum);
        return E_ERROR;
    }
    directory* dir=malloc(sizeof(directory));
    readi(current_dir,(uchar*)dir,0,sizeof(directory));

    //check existence
    int target_num=0;
    bool exist=false;
    for(int i=1;i<dir->num;i++){
        // Log("follow name: %s, size: %d",dir->entries[i].name,sizeof(dir->entries[i].name));
        if(strcmp(name,dir->entries[i].name)==0 && dir->entries[i].type==T_FILE){
            Log("File of same name exist: %s",name);
            target_num=i;
            exist=true;
            break;
        }
    }

    if(exist==false){
        Error("No such file: %s",name);
        iput(current_dir);
        free(dir);
        return E_ERROR;
    }

    //get the target
    inode* target=iget(dir->entries[target_num].inum);

    //read the file
    *buf=malloc(sizeof(uchar)*(target->size));
    readi(target,*buf,0,(target->size));
    *len=target->size;


    iput(target);
    iput(current_dir);
    free(dir);
    return E_SUCCESS;
}

int cmd_w(char *name, uint len, const char *data) {
    inode* current_dir=iget(sb.current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",sb.current_dir_inum);
        return E_ERROR;
    }
    if(strlen(data)>len){
        Error("Wrong len: %d",len);
        return E_ERROR;
    }
    directory* dir=malloc(sizeof(directory));
    readi(current_dir,(uchar*)dir,0,sizeof(directory));

    //check existence
    int target_num=0;
    bool exist=false;
    for(int i=1;i<dir->num;i++){
        // Log("follow name: %s, size: %d",dir->entries[i].name,sizeof(dir->entries[i].name));
        if(strcmp(name,dir->entries[i].name)==0 && dir->entries[i].type==T_FILE){
            Log("File of same name exist: %s",name);
            target_num=i;
            exist=true;
            break;
        }
    }

    if(exist==false){
        Error("No such file: %s",name);
        iput(current_dir);
        free(dir);
        return E_ERROR;
    }

    //get the target
    inode* target=iget(dir->entries[target_num].inum);

    //write the target
    writei(target,(uchar*)data,0,len);

    //update dir info
    dir->entries[target_num].size=target->size;
    dir->entries[target_num].update_time=target->modify_time;
    dir->entries[target_num].user=target->owner;
    dir->entries[target_num].mode=target->mode;
    strcpy(dir->entries[target_num].name,name);
    writei(current_dir,(uchar*)dir,0,sizeof(directory));



    iupdate(target);
    iput(target);
    iput(current_dir);
    free(dir);
    return E_SUCCESS;
}

int cmd_i(char *name, uint pos, uint len, const char *data) {
    inode* current_dir=iget(sb.current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",sb.current_dir_inum);
        return E_ERROR;
    }
    directory* dir=malloc(sizeof(directory));
    readi(current_dir,(uchar*)dir,0,sizeof(directory));

    //check existence
    int target_num=0;
    bool exist=false;
    for(int i=1;i<dir->num;i++){
        // Log("follow name: %s, size: %d",dir->entries[i].name,sizeof(dir->entries[i].name));
        if(strcmp(name,dir->entries[i].name)==0 && dir->entries[i].type==T_FILE){
            Log("File of same name exist: %s",name);
            target_num=i;
            exist=true;
            break;
        }
    }

    if(exist==false){
        Error("No such file: %s",name);
        iput(current_dir);
        free(dir);
        return E_ERROR;
    }

    //get the target
    inode* target=iget(dir->entries[target_num].inum);

    uint new_pos=min(target->size,pos);
    uint ori_size=target->size;

    //insert
    uchar* data_moved=malloc(sizeof(uchar)*(target->size-new_pos));
    readi(target,data_moved,new_pos,target->size-new_pos);
    writei(target,(uchar*)data,new_pos,len);
    writei(target,data_moved,new_pos+len,ori_size-new_pos);

    //update dir info
    dir->entries[target_num].size=target->size;
    dir->entries[target_num].update_time=target->modify_time;
    dir->entries[target_num].user=target->owner;
    dir->entries[target_num].mode=target->mode;
    strcpy(dir->entries[target_num].name,name);
    writei(current_dir,(uchar*)dir,0,sizeof(directory));

    iupdate(target);
    iput(target);
    free(data_moved);
    iput(current_dir);
    free(dir);
    return E_SUCCESS;
}

int cmd_d(char *name, uint pos, uint len) {
    inode* current_dir=iget(sb.current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",sb.current_dir_inum);
        return E_ERROR;
    }
    directory* dir=malloc(sizeof(directory));
    readi(current_dir,(uchar*)dir,0,sizeof(directory));

    //check existence
    int target_num=0;
    bool exist=false;
    for(int i=1;i<dir->num;i++){
        // Log("follow name: %s, size: %d",dir->entries[i].name,sizeof(dir->entries[i].name));
        if(strcmp(name,dir->entries[i].name)==0 && dir->entries[i].type==T_FILE){
            Log("File of same name exist: %s",name);
            target_num=i;
            exist=true;
            break;
        }
    }

    if(exist==false){
        Error("No such file: %s",name);
        iput(current_dir);
        free(dir);
        return E_ERROR;
    }

    //get the target
    inode* target=iget(dir->entries[target_num].inum);

    //delete the data
    int tmp=target->size-pos-len;
    uint data_moved_len=max(tmp,0);
    Log("data moved len: %d",data_moved_len);

    if(data_moved_len!=0){
        uchar* data_moved=malloc(sizeof(uchar)*data_moved_len);
        readi(target,data_moved,pos+len,data_moved_len);
        writei(target,data_moved,pos,data_moved_len);
        changesize(target,data_moved_len+pos);
        free(data_moved);
    }
    else{
        changesize(target,pos);
    }

    //update dir info
    dir->entries[target_num].size=target->size;
    dir->entries[target_num].update_time=target->modify_time;
    dir->entries[target_num].user=target->owner;
    dir->entries[target_num].mode=target->mode;
    strcpy(dir->entries[target_num].name,name);
    writei(current_dir,(uchar*)dir,0,sizeof(directory));
    

    iupdate(target);
    iput(target);
    iput(current_dir);
    free(dir);
    return E_SUCCESS;
}

int cmd_login(int auid) {

    sbinit();
    Log("sbinit");
    if(sb.magic!=0x6657){
        initialize_superblock_no_user(10,10,0);
    }
    bool exist=false;
    for(int i=0;i<sb.user_num;i++){
        if(sb.user_id[i]==auid){
            exist=true;
            break;
        }
    }
    if(exist==false){
        sb.user_id[sb.user_num]=auid;
        sb.user_num++;
    }

    update_superblock();
    Log("cmd_login: %d",auid);
    return E_SUCCESS;
}
