#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include "block.h"
#include "common.h"
#include "fs.h"
#include "log.h"
#include "tcp_utils.h"


// global variables
int ncyl, nsec;
#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))
int g_auid = -1; // current user ID, -1 means no user logged in

sem_t sem_cmd, sem_vector;


// handle functions
int handle_f(tcp_buffer *wb,char *args, int len, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);

    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd=cmd_f_i(auid, ncyl, nsec);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    }
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_PERMISSION_DENIED){
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

int handle_mk(tcp_buffer *wb,char *args, int len, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);


    char *name=strtok(args, " \r\n");
    if (name == NULL) {
        char err[]="Missing file name";
        Error("Missing file name");
        reply_with_no(wb, err, strlen(err)+1);
        return 0;
    }
    char* m=strtok(NULL, " \r\n");
    if (m == NULL) {
        char err[]="Missing mode name";
        Error("Missing mode name");
        reply_with_no(wb, err, strlen(err)+1);
        return 0;
    }
    short mode = atoi(m);

    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd= cmd_mk_i(auid, name, mode);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } 
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else {
        char no[]="Failed to create file";
        Error("Failed to create file");
        reply_with_no(wb, no, strlen(no)+1);
    }
    return 0;
}

int handle_mkdir(tcp_buffer *wb, char *args, int len, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    char *name=strtok(args, " \r\n");
    if (name == NULL) {
        char err[]="Missing dir name";
        Error("Missing dir name");
        reply_with_no(wb, err, strlen(err)+1);
        return 0;
    }
    char* m=strtok(NULL, " \r\n");
    if (m == NULL) {
        char err[]="Missing mode name";
        Error("Missing mode name");
        reply_with_no(wb, err, strlen(err)+1);
        return 0;
    }
    short mode = atoi(m);
    if (mode == -1 || mode > 0777) {
        char err[] = "Invalid mode";
        Error("Invalid mode");
        reply_with_no(wb, err, strlen(err) + 1);
        return 0;
    }
    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd = cmd_mkdir_i(auid, name, mode);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        reply_with_yes(wb,NULL,0);
    }
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else {
        char no[]="Failed to create dir";
        Error("Failed to create dir");
        reply_with_no(wb, no, strlen(no)+1);
    }
    return 0;
}

int handle_rm(tcp_buffer* wb, char *args, int len, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    char *name=strtok(args, " \r\n");
    if (name == NULL) {
        char err[]="Missing file name";
        Error("Missing file name");
        reply_with_no(wb, err, strlen(err)+1);
        return 0;
    }
    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd=cmd_rm_i(auid, name);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    }
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else {
        char no[]="Failed to remove file";
        Error("Failed to remove file");
        reply_with_no(wb, no, strlen(no)+1);
    }
    return 0;
}

int handle_cd(tcp_buffer *wb, char *args, int len, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    char *name=strtok(args, " \r\n");
    if (name == NULL) {
        char err[]="Missing dir name";
        Error("Missing dir name");
        reply_with_no(wb, err, strlen(err)+1);
        return 0;
    }
    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd=cmd_cd_i(auid, name);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    }
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    } 
    else {
        char no[]="Failed to change directory";
        Error("Failed to change directory");
        reply_with_no(wb, no, strlen(no)+1);
    }
    return 0;
}

int handle_rmdir(tcp_buffer *wb, char *args, int len, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    char *name=strtok(args, " \r\n");
    if (name == NULL) {
        char err[]="Missing dir name";
        Error("Missing dir name");
        reply_with_no(wb, err, strlen(err)+1);
        return 0;
    }
    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd=cmd_rmdir_i(auid, name);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    }
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    }  
    else {
        char no[]="Failed to remove directory";
        Error("Failed to remove directory");
        reply_with_no(wb, no, strlen(no)+1);
    }
    return 0;
}

int handle_ls(tcp_buffer *wb, char *args, int len, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    entry *entries = NULL;
    int n = 0;
    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd = cmd_ls_i(auid, &entries, &n);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if(cmd==E_ERROR){
        char no[] = "Failed to list files";
        Error("Failed to list files");
        reply_with_no(wb, no, strlen(no) + 1);
        return 0;
    }
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
        return 0;
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
        return 0;
    }
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
        return 0;
    }
    char *buf = NULL;
    int buf_len = 0;
    for (int i = 0; i < n; i++) {
        char *name = entries[i].name;
        uint type = entries[i].type;
        uint size = entries[i].size;
        uint update_time = entries[i].update_time;
        int user = entries[i].user;
        char temp[256];
        if (type == T_DIR) {
            if(i != n-1){
                snprintf(temp, sizeof(temp), "name: %s type: Dir updated_time: %d user: %d\n", name, update_time,user);
            }else{
                snprintf(temp, sizeof(temp), "name: %s type: Dir updated_time: %d user: %d", name, update_time, user);
            }
            
        } else {
            if(i != n-1){
                snprintf(temp, sizeof(temp), "name: %s type: File size: %d updated_time: %d user: %d\n", name, size, update_time, user);
            }else{
                snprintf(temp, sizeof(temp), "name: %s type: File size: %d updated_time: %d user: %d", name, size, update_time, user);
            }
        }
        int new_len = buf_len + strlen(temp);
        char *new_buf = realloc(buf, new_len + 1);
        if (new_buf != NULL) {
            buf = new_buf;
            strcpy(buf + buf_len, temp);
            buf_len = new_len;
        }
    }
    if (buf != NULL) {
        char *new_buf = realloc(buf, buf_len + 1);
        if (new_buf != NULL) {
            buf = new_buf;
            buf[buf_len] = '\0';
        }
        reply_with_yes(wb, buf, buf_len + 1);
        free(buf);
    }
    else{
        char no[] = "No file or dir";
        reply_with_yes(wb, no, strlen(no) + 1);
        free(entries);
        return 0;
    }
    free(entries);
    return 0;
}

int handle_cat(tcp_buffer *wb, char *args, int lenth, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    char *name=strtok(args, " \r\n");
    if (name == NULL) {
        char error[]="Missing file name";
        Error("Missing file name");
        reply_with_no(wb, error, strlen(error)+1);
        return 0;
    }
    uchar *buf = NULL;
    uint len;
    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd= cmd_cat_i(auid, name, &buf, &len);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        Log("length: %d", len);
        uchar* tmp=malloc(sizeof(uchar)*(len+1));
        for(int i=0;i<len;i++){
            tmp[i]=buf[i];
        }
        tmp[len]='\0';
        reply_with_yes(wb, (char*)tmp, len+1);
    
        free(tmp);
        free(buf);
    } 
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    } 
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    } 
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    } 
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else {
        char no[] = "Failed to read files";
        Error("Failed to read files");
        reply_with_no(wb, no, strlen(no) + 1);
        return 0;
    }
    return 0;
}

int handle_w(tcp_buffer *wb, char *args, int lenth, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    char *name;
    uint len;
    char *data;
    char *saveptr;
    int offset = 0;

    name = __strtok_r(args, " \r\n", &saveptr);
    offset += strlen(name) + 1; 
    if (name == NULL) {
        char error[] = "Missing file name";
        Error("Missing file name");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    char *len_str = __strtok_r(NULL, " \r\n", &saveptr);
    offset += strlen(len_str) + 1;
    if (len_str == NULL) {
        char error[] = "Missing length";
        Error("Missing length");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    data=args+offset;

    if (data == NULL) {
        char error[] = "Missing data";
        Error("Missing data");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    len = atoi(len_str);
    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd=cmd_w_i(auid, name, len, data);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    }
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else {
        char no[] = "Failed to write file";
        Error("Failed to write file");
        reply_with_no(wb, no, strlen(no) + 1);
    }
    return 0;
}


int handle_i(tcp_buffer *wb, char *args, int lenth, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    char *name;
    uint pos;
    uint len;
    char *data;
    name=strtok(args, " \r\n");
    if (name == NULL) {
        char error[] = "Missing file name";
        Error("Missing file name");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    char* pos_str=strtok(NULL, " \r\n");
    if (pos_str == NULL) {
        char error[] = "Missing position";
        Error("Missing position");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    char* len_str=strtok(NULL, " \r\n");
    if (len_str == NULL) {
        char error[] = "Missing length";
        Error("Missing length");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    data = strtok(NULL, " \r\n");
    if (data == NULL) {
        char error[] = "Missing data";
        Error("Missing data");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    pos=atoi(pos_str);
    len=atoi(len_str);
    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd=cmd_i_i(auid, name, pos, len, data);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    }
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else {
        char no[] = "Failed to write file";
        Error("Failed to write file");
        reply_with_no(wb, no, strlen(no) + 1);
    }
    return 0;
}

int handle_d(tcp_buffer *wb, char *args, int lenth, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    char *name;
    uint pos;
    uint len;
    name=strtok(args, " \r\n");
    if (name == NULL) {
        char error[] = "Missing file name";
        Error("Missing file name");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    char* pos_str=strtok(NULL, " \r\n");
    if (pos_str == NULL) {
        char error[] = "Missing position";
        Error("Missing position");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    char* len_str=strtok(NULL, " \r\n");
    if (len_str == NULL) {
        char error[] = "Missing length";
        Error("Missing length");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    pos=atoi(pos_str);
    len=atoi(len_str);
    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd=cmd_d_i(auid, name, pos, len);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } 
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else {
        char no[] = "Failed to delete file";
        Error("Failed to delete file");
        reply_with_no(wb, no, strlen(no) + 1);
    }
    return 0;
}

int handle_e(tcp_buffer *wb, char *args, int lenth, int id) {
    char msg[] = "Bye!";
    reply(wb, msg, strlen(msg) + 1);
    Log("Exit");
    sem_wait(&sem_vector);
    client_logout(id); // logout the client
    sem_post(&sem_vector);
    return -1;
}

int handle_login(tcp_buffer *wb, char *args, int lenth, int id) {
    if(args == NULL || strlen(args) == 0) {
        char error[] = "Missing user ID";
        Error("Missing user ID");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    int uid=atoi(strtok(args, " \r\n"));
    sem_wait(&sem_vector);
    int cur_uid= get_uid(id);
    if(cur_uid != -1) {
        char error[] = "Already logged in with another user";
        Error("Already logged in with another user");
        reply_with_no(wb, error, strlen(error) + 1);
        sem_post(&sem_vector);
        return 0;
    }
    sem_post(&sem_vector);

    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    if (cmd_login(uid) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
        Log("User %d logged in", uid);
        // g_auid = uid; // set current user ID
        update_client_id(uid, id); // update client ID in the server
    } 
    else {
        char no[] = "Failed to login";
        Error("Failed to login");
        reply_with_no(wb, no, strlen(no) + 1);
    }
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    return 0;
}

int handle_logout(tcp_buffer *wb, char *args, int lenth, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);

    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    if (cmd_logout(auid) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
        Log("User %d logged out", auid);
    } else {
        char no[] = "Failed to logout";
        Error("Failed to logout");
        reply_with_no(wb, no, strlen(no) + 1);
    }
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    return 0;
}

int handle_pwd(tcp_buffer *wb, char *args, int lenth, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    char name[512];
    int len;
    sem_wait(&sem_cmd);
    sem_wait(&sem_vector);
    int cmd= cmd_pwd_i(auid, name, &len);
    sem_post(&sem_vector);
    sem_post(&sem_cmd);
    if (cmd == E_SUCCESS) {
        reply_with_yes(wb, name, strlen(name) + 1);
        Log("Current directory: %s", name);
    }
    else if(cmd == E_NOT_LOGGED_IN) {
        char err[] = "User not logged in";
        Error("User not logged in");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_ERROR) {
        char err[] = "Error occurred";
        Error("Error occurred");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_PERMISSION_DENIED) {
        char err[] = "Permission denied";
        Error("Permission denied");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else if(cmd == E_NOT_FORMATTED) {
        char err[] = "Disk not formatted";
        Error("Disk not formatted");
        reply_with_no(wb, err, strlen(err) + 1);
    }
    else {
        char no[] = "Failed to get current directory";
        Error("Failed to get current directory");
        reply_with_no(wb, no, strlen(no) + 1);
    }

    return 0;
}

//for debug
int handle_show(tcp_buffer *wb, char *args, int lenth, int id) {
    sem_wait(&sem_vector);
    int auid = get_uid(id);
    sem_post(&sem_vector);
    char msg[256];
    snprintf(msg, sizeof(msg), "Current user ID: %d", auid);
    reply(wb, msg, strlen(msg) + 1);
    Log("client %d requested to show current user ID", id);
    //Log current vector
    sem_wait(&sem_vector);
    show_vector();
    sem_post(&sem_vector);
    return 0;
}

static struct {
    const char *name;
    int (*handler)(tcp_buffer *wb, char *, int, int);
} cmd_table[] = {{"f", handle_f},        {"mk", handle_mk},       {"mkdir", handle_mkdir}, {"rm", handle_rm},
                 {"cd", handle_cd},      {"rmdir", handle_rmdir}, {"ls", handle_ls},       {"cat", handle_cat},
                 {"w", handle_w},        {"i", handle_i},         {"d", handle_d},         {"e", handle_e},
                 {"login", handle_login}, {"logout", handle_logout}, {"pwd", handle_pwd}, {"show", handle_show}};

void on_connection(int id) {
    // some code that are executed when a new client is connected
}

int on_recv(int id, tcp_buffer *wb, char *msg, int len) {

    char *p = strtok(msg, " \r\n");
    int ret = 1;
    for (int i = 0; i < NCMD; i++)
        if (p && strcmp(p, cmd_table[i].name) == 0) {
            ret = cmd_table[i].handler(wb, p + strlen(p) + 1, len - strlen(p) - 1, id);
            break;
        }
    if (ret == 1) {
        static char unk[] = "Unknown command";
        buffer_append(wb, unk, sizeof(unk));
    }
    if (ret < 0) {
        return -1;
    }
    return 0;
}

void cleanup(int id) {
    // some code that are executed when a client is disconnected
    sem_wait(&sem_vector);
    client_logout(id); // logout the user
    sem_post(&sem_vector);
    Log("Client %d disconnected", id);
}

FILE *log_file;

int main(int argc, char *argv[]) {
    if(argc < 3) {
        fprintf(stderr, "Usage: %s <Disk System Port> <File System Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int disk_port = atoi(argv[1]);
    int port = atoi(argv[2]);


    sem_init(&sem_cmd, 0, 1);
    sem_init(&sem_vector, 0, 1);

    log_init("fs.log");
    block_client_init(disk_port);
    Log("File System Server started on port %d", port);
    // get disk info and store in global variables
    get_disk_info(&ncyl, &nsec);


    sbinit();
    // command
    tcp_server server = server_init(port, 5, on_connection, on_recv, cleanup);
    server_run(server);

    // never reached
    log_close();
    sem_destroy(&sem_cmd);
    sem_destroy(&sem_vector);
}
