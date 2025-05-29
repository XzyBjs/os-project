#include "fs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "block.h"
#include "log.h"

users_vector users;

//mode -1: no one can modify
//mode -2: only owner can modify


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
    Log("sbinit");
}

int cmd_f_i(int auid, int ncyl, int nsec) {
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid!=1){
        return E_PERMISSION_DENIED;
    }
    sbinit();
    Log("cmd_f: ncyl=%d, nsec=%d", ncyl, nsec);
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

        user->owner=sb.user_id[i-1];
        user->group=0;
        user->mode=-2;
        user->modify_time=time(NULL);
        user->creation_time=user->modify_time;

        home_dir->entries[i].inum=user->inum;
        home_dir->entries[i].update_time=user->modify_time;
        home_dir->entries[i].user=sb.user_id[i-1];
        home_dir->entries[i].mode=-1;
        strcpy(user->name,user_name);

        iupdate(user);
        free(user_dir);
        iput(user);
    }
    writei(home,(uchar*)home_dir,0,sizeof(directory));

    sb.current_dir_inum=root->inum;
    sb.format=1;

    initialize_users_vector();
    add_user(1); // add root user

    iupdate(home);
    iupdate(root);
    iput(home);
    iput(root);
    free(home_dir);
    free(root_dir);
    update_superblock();
    Log("cmd_format");
    Log("Superblock initialized: magic=0x%x, size=%d, bmapstart=%d, bmapend=%d, imapstart=%d, imapend=%d, inode_start=%d, inode_end=%d, data_start=%d, data_block_count=%d, data_block_free=%d, inode_count=%d, inode_free=%d", 
        sb.magic, sb.size, sb.bmapstart, sb.bmapend, sb.imapstart, sb.imapend,
        sb.inode_start, sb.inode_end, sb.data_start, sb.data_block_count,
        sb.data_block_free, sb.inode_count, sb.inode_free);
    return E_SUCCESS;
}

int cmd_mk_i(int auid, char *name, short mode) {
    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(mode==-1){
        Error("Mode -1 is not allowed");
        return E_PERMISSION_DENIED;
    }
    //get current dir of user
    uint current_dir_inum;
    get_user_inum(auid, &current_dir_inum);
    inode* current_dir=iget(current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",current_dir_inum);
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
    new_file->owner=auid;
    new_file->group=0;
    new_file->modify_time=time(NULL);
    new_file->creation_time=new_file->modify_time;
    strcpy(new_file->name,name);

    //update dir
    dir->entries[dir->num].inum=new_file->inum;
    dir->entries[dir->num].mode=mode;
    dir->entries[dir->num].size=0;
    dir->entries[dir->num].type=T_FILE;
    dir->entries[dir->num].update_time=new_file->modify_time;
    dir->entries[dir->num].user=auid;
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

int cmd_mkdir_i(int auid, char *name, short mode) {

    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(mode==-1){
        Error("Mode -1 is not allowed");
        return E_PERMISSION_DENIED;
    }
    //get current dir of user
    uint current_dir_inum;
    get_user_inum(auid, &current_dir_inum);
    inode* current_dir=iget(current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",current_dir_inum);
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
    new_dir_i->owner=auid;
    new_dir_i->group=0;
    new_dir_i->modify_time=time(NULL);
    new_dir_i->creation_time=new_dir_i->modify_time;

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
    dir->entries[dir->num].user=auid;
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

int cmd_rm_i(int auid, char *name) {
    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    //get current dir of user
    uint current_dir_inum;
    get_user_inum(auid, &current_dir_inum);
    inode* current_dir=iget(current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",current_dir_inum);
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
    inode* target=iget(dir->entries[target_num].inum);
    if(target->mode==-1){
        Error("File is not allowed to delete: %s",name);
        iput(current_dir);
        free(dir);
        return E_PERMISSION_DENIED;
    }

    if(target->mode==-2 && target->owner!=auid){
        Error("Permission denied: %s",name);
        iput(current_dir);
        free(dir);
        return E_PERMISSION_DENIED;
    }

    //delete file
    deletei(target);


    //update current dir
    for(int i=target_num+1;i<dir->num;i++){
        dir->entries[i-1].inum=dir->entries[i].inum;
        dir->entries[i-1].mode=dir->entries[i].mode;
        dir->entries[i-1].size=dir->entries[i].size;
        dir->entries[i-1].type=dir->entries[i].type;
        dir->entries[i-1].update_time=dir->entries[i-1].update_time;
        dir->entries[i-1].user=dir->entries[i].user;
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

int cmd_cd_i(int auid, char *name) {
    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }

    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    char final_name[BSIZE]; // used to update user's current dir name

    char* tmp=malloc(sizeof(char)*100);
    strcpy(tmp,name);
    inode* current_dir=NULL;
    directory* dir=malloc(sizeof(directory));
    char* token=NULL;
    bool absolute=(tmp[0]=='/')?1:0;
    if(tmp[0]==0){
        return E_SUCCESS; //no change
    }
    if(absolute){
        //get root
        current_dir=iget(0);
        strcpy(final_name,"/");
        token=strtok(tmp+1,"/");
        
    }
    else{
        //get current dir of user
        uint current_dir_inum;
        get_user_inum(auid, &current_dir_inum);
        //update final_name
        get_user_dname(auid, final_name);
        
        current_dir=iget(current_dir_inum);

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
        if(current_dir->mode==-2 && current_dir->owner!=auid){
            Error("Permission denied: %s, owner: %d",name, current_dir->owner);
            iput(current_dir);
            free(dir);
            return E_PERMISSION_DENIED;
        }
        //update final_name by adding the token
        //condiser the case of ..
        if(strcmp(token,"..")==0){
            //remove the last part of final_name
            char* last_slash=strrchr(final_name, '/');
            if(last_slash != NULL) {
                *last_slash = '\0'; // remove the last part
            }
            if(strlen(final_name)==0){
                strcpy(final_name,"/");
            }
        }
        else if(strlen(final_name)>1){
            strcat(final_name,"/");
            strcat(final_name, token);
        }
        else{
            strcat(final_name, token); // if final_name is empty, just copy the token
        }
        token=strtok(NULL,"/");
    }
    

    //update user current dir
    change_user_inum(auid, current_dir->inum);
    change_user_dname(auid, final_name);

    
    update_superblock();
    iput(current_dir);
    free(dir);
    free(tmp);
    return E_SUCCESS;
}

int cmd_rmdir_i(int auid, char *name) {
    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    //get current dir of user
    uint current_dir_inum;
    get_user_inum(auid, &current_dir_inum);
    inode* current_dir=iget(current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",current_dir_inum);
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
    
    if(target->mode==-1){
        Error("Dir is not allowed to delete: %s",name);
        iput(current_dir);
        free(dir);
        return E_PERMISSION_DENIED;
    }
    if(target->mode==-2 && target->owner!=auid){
        Error("Permission denied: %s",name);
        iput(current_dir);
        free(dir);
        return E_PERMISSION_DENIED;
    }

    //check if other users are in this dir
   
    for(int i=0;i<5;i++){
        if(users.valid[i] && users.logstatus[i]==1 && users.current_dir_inum[i]==target->inum){
            Error("User %d is in this dir: %s",users.user_id[i],name);
            iput(current_dir);
            free(dir);
            free(target_dir);
            return E_PERMISSION_DENIED;
        }
        //move the user out of this dir if they logged out
        if(users.valid[i] && users.logstatus[i]==0 && users.current_dir_inum[i]==target->inum ){
            Log("User %d is moved out of dir: %s",users.user_id[i],name);
            change_user_inum(users.user_id[i], current_dir->inum);
            char current_dir_name[BSIZE];
            get_user_dname(auid, current_dir_name);
            change_user_dname(users.user_id[i], current_dir_name);
        }
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
        dir->entries[i-1].user=dir->entries[i].user;
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
int cmd_ls_i(int auid, entry **entries, int *n) {
    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    //get current dir of user
    uint current_dir_inum;
    get_user_inum(auid, &current_dir_inum);
    inode* current_dir = iget(current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",current_dir_inum);
        return E_ERROR;
    }
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

int cmd_cat_i(int auid, char *name, uchar **buf, uint *len) {
    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    //get current dir of user
    uint current_dir_inum;
    get_user_inum(auid, &current_dir_inum);
    inode* current_dir=iget(current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",current_dir_inum);
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

    if(target->mode==-2 && target->owner!=auid){
        Error("Permission denied: %s",name);
        iput(current_dir);
        free(dir);
        return E_PERMISSION_DENIED;
    }

    //read the file
    *buf=malloc(sizeof(uchar)*(target->size));
    readi(target,*buf,0,(target->size));
    *len=target->size;


    iput(target);
    iput(current_dir);
    free(dir);
    return E_SUCCESS;
}

int cmd_w_i(int auid, char *name, uint len, const char *data) {
    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    //get current dir of user
    uint current_dir_inum;
    get_user_inum(auid, &current_dir_inum);
    inode* current_dir=iget(current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",current_dir_inum);
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

    if(target->mode==-2 && target->owner!=auid){
        Error("Permission denied: %s",name);
        iput(current_dir);
        free(dir);
        return E_PERMISSION_DENIED;
    }

    //write the target
    writei(target,(uchar*)data,0,len);

    //update dir info
    dir->entries[target_num].size=target->size;
    dir->entries[target_num].update_time=target->modify_time;
    strcpy(dir->entries[target_num].name,name);
    writei(current_dir,(uchar*)dir,0,sizeof(directory));



    iupdate(target);
    iput(target);
    iput(current_dir);
    free(dir);
    return E_SUCCESS;
}

int cmd_i_i(int auid, char *name, uint pos, uint len, const char *data) {
    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    //get current dir of user
    uint current_dir_inum;
    get_user_inum(auid, &current_dir_inum);
    inode* current_dir=iget(current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",current_dir_inum);
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


    if(target->mode==-2 && target->owner!=auid){
        Error("Permission denied: %s",name);
        iput(current_dir);
        free(dir);
        return E_PERMISSION_DENIED;
    }
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

int cmd_d_i(int auid, char *name, uint pos, uint len) {
    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    //get current dir of user
    uint current_dir_inum;
    get_user_inum(auid, &current_dir_inum);
    inode* current_dir=iget(current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",current_dir_inum);
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

    if(target->mode==-2 && target->owner!=auid){
        Error("Permission denied: %s",name);
        iput(current_dir);
        free(dir);
        return E_PERMISSION_DENIED;
    }

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
    if(auid<0){
        Error("Invalid user ID: %d", auid);
        return E_PERMISSION_DENIED;
    }
    sbinit();
    Log("sbinit");
    if(sb.magic!=0x6657){
        initialize_superblock_no_user(1024,63,0);
    }
    int addr=check_user_addr(auid);
    if(addr==-1){
        add_user(auid);
        Log("User %d added", auid);
    } else {
        if(users.logstatus[addr]==1){
            Error("User %d already logged in", auid);
            return E_PERMISSION_DENIED;
        }
        users.logstatus[addr]=1; //set logged in
        Log("User %d logged in", auid);
    }
    
    bool exist=false;
    for(int i=0;i<sb.user_num;i++){
        if(sb.user_id[i]==auid){
            exist=true;
            break;
        }
    }
    if(exist==false){
        if(sb.user_num>=5){
            Error("Too many users: %d",sb.user_num);
            return E_ERROR;
        }
        sb.user_id[sb.user_num]=auid;
        sb.user_num++;
        if(sb.format==1){
            cmd_cd_i(auid, "/");
            cmd_cd_i(auid, "users");
            char user_name[MAXNAME];
            int_to_char(auid, user_name);
            cmd_mkdir_i(auid, user_name, -2);
            cmd_cd_i(auid, "..");
        }
    }

    update_superblock();
    Log("cmd_login: %d",auid);
    return E_SUCCESS;
}
int cmd_logout(int auid) {
    int addr=check_user_addr(auid);
    if(addr==-1){
        Error("User %d not logged in", auid);
        return E_ERROR;
    }
    if(users.logstatus[addr]==0){
        Error("User %d already logged out", auid);
        return E_ERROR;
    }
    users.logstatus[addr]=0; //set logged out
    Log("User %d logged out", auid);
    return E_SUCCESS;
}

int cmd_pwd_i(int auid, char *name, int *len){
    if(sb.magic!=0x6657 || sb.format==0){
        return E_NOT_FORMATTED;
    }
    if(check_user_addr(auid)==-1){
        Error("User not logged in");
        return E_NOT_LOGGED_IN;
    }
    if(auid<0){
        Error("Not logged in");
        return E_NOT_LOGGED_IN;
    }
    //get current dir of user
    uint current_dir_inum;
    get_user_inum(auid, &current_dir_inum);
    inode* current_dir=iget(current_dir_inum);
    if(current_dir==NULL){
        Error("Wrong current dir: %d",current_dir_inum);
        return E_ERROR;
    }

    //get user current dir name
    char tmp[BSIZE];
    get_user_dname(auid, tmp);
    
    strcpy(name, tmp);

    *len = strlen(name);

    iput(current_dir);
    Log("cmd_pwd_i: %s", name);
    return E_SUCCESS;
}

void add_user(int auid) {
    for(int i=0;i<5;i++){
        if(users.valid[i]==0){
            users.valid[i]=1;
            users.user_id[i]=auid;
            users.logstatus[i]=1; //logged in
            users.current_dir_inum[i]=0;
            strcpy(users.current_dir_name[i], "/");
            Log("User %d added at index %d", auid, i);
            return;
        }
    }
}

void initialize_users_vector() {
    for(int i=0;i<5;i++){
        users.valid[i]=0;
        users.user_id[i]=0;
        users.logstatus[i]=0; //logged out
        users.current_dir_inum[i]=0;
        strcpy(users.current_dir_name[i], "/");
    }
    Log("Users vector initialized");
} 

int check_user_addr(int auid){
    for(int i=0;i<5;i++){
        if(users.valid[i] && users.user_id[i]==auid){
            return i;
        }
    }
    return -1; //not found
}
void get_user_inum(int auid, uint *inum){
    int index = check_user_addr(auid);
    if(index != -1) {
        *inum = users.current_dir_inum[index];
    } else {
        *inum = 0; //default to root if user not found
    }
}

void get_user_dname(int auid, char *name){
    int index = check_user_addr(auid);
    if(index != -1) {
        strcpy(name, users.current_dir_name[index]);
    } else {
        strcpy(name, "/"); //default to root if user not found
    }
}

void change_user_inum(int auid, uint inum){
    int index = check_user_addr(auid);
    if(index != -1) {
        users.current_dir_inum[index] = inum;
        Log("User %d current dir inum changed to %d", auid, inum);
    } else {
        Error("User %d not found", auid);
    }
}

void change_user_dname(int auid, const char *name){
    int index = check_user_addr(auid);
    if(index != -1) {
        strcpy(users.current_dir_name[index], name);
        Log("User %d current dir name changed to %s", auid, users.current_dir_name[index]);
    } else {
        Error("User %d not found", auid);
    }
}

void update_client_id(int auid, int client_id) {
    int index = check_user_addr(auid);
    if(index != -1) {
        users.client_id[index] = client_id;
        Log("User %d client ID updated to %d", auid, client_id);
    } else {
        Error("User %d not found", auid);
    }
}

void client_logout(int client_id) {
    for(int i=0;i<5;i++){
        if(users.valid[i] && users.client_id[i]==client_id){
            users.valid[i]=0; // mark user as logged out
            Log("Client %d logged out user %d", client_id, users.user_id[i]);
        }
    }
}

int get_client_id(int auid) {
    int index = check_user_addr(auid);
    if(index != -1) {
        return users.client_id[index];
    } else {
        Error("User %d not found", auid);
        return -1; // user not found
    }
}

int get_uid(int client_id) {
    for(int i=0;i<5;i++){
        if(users.valid[i] && users.client_id[i]==client_id && users.logstatus[i]==1){
            return users.user_id[i];
        }
    }

    return -1; 
}

int show_vector() {
    Log("Users vector:");
    for(int i=0;i<5;i++){
        Log("Valid: %d, Index: %d, User ID: %d, Log Status: %d, Current Dir Inum: %d, Current Dir Name: %s, Client ID: %d", users.valid[i],
            i, users.user_id[i], users.logstatus[i], users.current_dir_inum[i], users.current_dir_name[i], users.client_id[i]);

    }
    return E_SUCCESS;
}