#ifndef __FS_H__
#define __FS_H__

#include "common.h"
#include "inode.h"

// used for cmd_ls
typedef struct {
    short type;
    char name[MAXNAME];
    uint size;
    uint inum;
    short user;
    short mode;
    uint update_time;
    // ...
    // ...
    // Other fields can be added as needed
} entry;




typedef struct {
    uint num;
    entry entries[15];
    uint padding[7];
}directory;


typedef struct {
    bool valid[5];
    int user_id[5];
    int logstatus[5]; // 0: logged out, 1: logged in
    uint current_dir_inum[5];
    char current_dir_name[5][BSIZE];
    int client_id[5]; // for multi-client support
    // Other fields can be added as needed
}users_vector;

extern users_vector users;

void get_user_inum(int auid, uint *inum);
void get_user_dname(int auid, char *name);
void change_user_inum(int auid, uint inum);
void change_user_dname(int auid, const char *name);
int check_user_addr(int auid);// return addr if exist, -1 if not exist
void initialize_users_vector();
void add_user(int auid); //add if not exist
void update_client_id(int auid, int id);
void client_logout(int id);
int get_client_id(int auid);
int get_uid(int id);


void sbinit();

int cmd_f(int ncyl, int nsec);

int cmd_mk(char *name, short mode);
int cmd_mkdir(char *name, short mode);
int cmd_rm(char *name);
int cmd_rmdir(char *name);

int cmd_cd(char *name);
int cmd_ls(entry **entries, int *n);

int cmd_cat(char *name, uchar **buf, uint *len);
int cmd_w(char *name, uint len, const char *data);
int cmd_i(char *name, uint pos, uint len, const char *data);
int cmd_d(char *name, uint pos, uint len);

int cmd_f_i(int auid, int ncyl, int nsec);

int cmd_mk_i(int auid, char *name, short mode);
int cmd_mkdir_i(int auid, char *name, short mode);
int cmd_rm_i(int auid, char *name);
int cmd_rmdir_i(int auid, char *name);

int cmd_cd_i(int auid, char *name);
int cmd_ls_i(int auid, entry **entries, int *n);

int cmd_cat_i(int auid, char *name, uchar **buf, uint *len);
int cmd_w_i(int auid, char *name, uint len, const char *data);
int cmd_i_i(int auid, char *name, uint pos, uint len, const char *data);
int cmd_d_i(int auid, char *name, uint pos, uint len);
int cmd_pwd_i(int auid, char *buf, int *len);

int cmd_login(int auid);
int cmd_logout(int auid);

int show_vector();
#endif