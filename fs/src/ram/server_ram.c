#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "block.h"
#include "common.h"
#include "fs.h"
#include "log.h"
#include "tcp_utils.h"


// global variables
int ncyl, nsec;
#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))





// handle functions
int handle_f(tcp_buffer *wb,char *args, int len) {
    if (cmd_f(ncyl, nsec) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

int handle_mk(tcp_buffer *wb,char *args, int len) {
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
    if (cmd_mk(name, mode) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        char no[]="Failed to create file";
        Error("Failed to create file");
        reply_with_no(wb, no, strlen(no)+1);
    }
    return 0;
}

int handle_mkdir(tcp_buffer *wb, char *args, int len) {
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
    if (cmd_mkdir(name, mode) == E_SUCCESS) {
        reply_with_yes(wb,NULL,0);
    } else {
        char no[]="Failed to create dir";
        Error("Failed to create dir");
        reply_with_no(wb, no, strlen(no)+1);
    }
    return 0;
}

int handle_rm(tcp_buffer* wb, char *args, int len) {
    char *name=strtok(args, " \r\n");
    if (name == NULL) {
        char err[]="Missing file name";
        Error("Missing file name");
        reply_with_no(wb, err, strlen(err)+1);
        return 0;
    }
    if (cmd_rm(name) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        char no[]="Failed to remove file";
        Error("Failed to remove file");
        reply_with_no(wb, no, strlen(no)+1);
    }
    return 0;
}

int handle_cd(tcp_buffer *wb, char *args, int len) {
    char *name=strtok(args, " \r\n");
    if (name == NULL) {
        char err[]="Missing dir name";
        Error("Missing dir name");
        reply_with_no(wb, err, strlen(err)+1);
        return 0;
    }
    if (cmd_cd(name) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        char no[]="Failed to change directory";
        Error("Failed to change directory");
        reply_with_no(wb, no, strlen(no)+1);
    }
    return 0;
}

int handle_rmdir(tcp_buffer *wb, char *args, int len) {
    char *name=strtok(args, " \r\n");
    if (name == NULL) {
        char err[]="Missing dir name";
        Error("Missing dir name");
        reply_with_no(wb, err, strlen(err)+1);
        return 0;
    }
    if (cmd_rmdir(name) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        char no[]="Failed to remove directory";
        Error("Failed to remove directory");
        reply_with_no(wb, no, strlen(no)+1);
    }
    return 0;
}

int handle_ls(tcp_buffer *wb, char *args, int len) {
    entry *entries = NULL;
    int n = 0;
    if (cmd_ls(&entries, &n) != E_SUCCESS) {
        char no[] = "Failed to list files";
        Error("Failed to list files");
        reply_with_no(wb, no, strlen(no) + 1);
        return 0;
    }
    char *buf = NULL;
    int buf_len = 0;
    for (int i = 0; i < n; i++) {
        char *name = entries[i].name;
        uint type = entries[i].type;
        uint size = entries[i].size;
        uint update_time = entries[i].update_time;
        char temp[256];
        if (type == T_DIR) {
            if(i != n-1){
                snprintf(temp, sizeof(temp), "name: %s type: Dir updated_time: %d \n", name, update_time);
            }else{
                snprintf(temp, sizeof(temp), "name: %s type: Dir updated_time: %d ", name, update_time);
            }
            
        } else {
            if(i != n-1){
                snprintf(temp, sizeof(temp), "name: %s type: File size: %d updated_time: %d \n", name, size, update_time);
            }else{
                snprintf(temp, sizeof(temp), "name: %s type: File size: %d updated_time: %d", name, size, update_time);
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

int handle_cat(tcp_buffer *wb, char *args, int lenth) {
    char *name=strtok(args, " \r\n");
    if (name == NULL) {
        char error[]="Missing file name";
        Error("Missing file name");
        reply_with_no(wb, error, strlen(error)+1);
        return 0;
    }
    uchar *buf = NULL;
    uint len;
    if (cmd_cat(name, &buf, &len) == E_SUCCESS) {
        uchar* tmp=malloc(sizeof(uchar)*(len+1));
        for(int i=0;i<len;i++){
            tmp[i]=buf[i];
        }
        tmp[len]='\0';
        reply_with_yes(wb, (char*)tmp, len+1);
    
        free(tmp);
        free(buf);
    } else {
        char no[] = "Failed to read files";
        Error("Failed to read files");
        reply_with_no(wb, no, strlen(no) + 1);
        return 0;
    }
    return 0;
}

int handle_w(tcp_buffer *wb, char *args, int lenth) {
    char *name;
    uint len;
    char *data;
    name = strtok(args, " \r\n");
    if (name == NULL) {
        char error[] = "Missing file name";
        Error("Missing file name");
        reply_with_no(wb, error, strlen(error) + 1);
        return 0;
    }
    char *len_str = strtok(NULL, " \r\n");
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
    len = atoi(len_str);
    if (cmd_w(name, len, data) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        char no[] = "Failed to write file";
        Error("Failed to write file");
        reply_with_no(wb, no, strlen(no) + 1);
    }
    return 0;
}

int handle_i(tcp_buffer *wb, char *args, int lenth) {
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
    if (cmd_i(name, pos, len, data) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        char no[] = "Failed to write file";
        Error("Failed to write file");
        reply_with_no(wb, no, strlen(no) + 1);
    }
    return 0;
}

int handle_d(tcp_buffer *wb, char *args, int lenth) {
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

    if (cmd_d(name, pos, len) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        char no[] = "Failed to delete file";
        Error("Failed to delete file");
        reply_with_no(wb, no, strlen(no) + 1);
    }
    return 0;
}

int handle_e(tcp_buffer *wb, char *args, int lenth) {
    char msg[] = "Bye!";
    reply(wb, msg, strlen(msg) + 1);
    Log("Exit");
    return -1;
}

int handle_login(tcp_buffer *wb, char *args, int lenth) {
    int uid=atoi(strtok(args, " \r\n"));
    if (cmd_login(uid) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
        Log("User %d logged in", uid);
    } else {
        char no[] = "Failed to login";
        Error("Failed to login");
        reply_with_no(wb, no, strlen(no) + 1);
    }
    return 0;
}

static struct {
    const char *name;
    int (*handler)(tcp_buffer *wb, char *, int);
} cmd_table[] = {{"f", handle_f},        {"mk", handle_mk},       {"mkdir", handle_mkdir}, {"rm", handle_rm},
                 {"cd", handle_cd},      {"rmdir", handle_rmdir}, {"ls", handle_ls},       {"cat", handle_cat},
                 {"w", handle_w},        {"i", handle_i},         {"d", handle_d},         {"e", handle_e},
                 {"login", handle_login}};

void on_connection(int id) {
    // some code that are executed when a new client is connected
}

int on_recv(int id, tcp_buffer *wb, char *msg, int len) {

    char *p = strtok(msg, " \r\n");
    int ret = 1;
    for (int i = 0; i < NCMD; i++)
        if (p && strcmp(p, cmd_table[i].name) == 0) {
            ret = cmd_table[i].handler(wb, p + strlen(p) + 1, len - strlen(p) - 1);
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
}

FILE *log_file;

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);


    log_init("fs.log");

    // get disk info and store in global variables
    // get_disk_info(&ncyl, &nsec);
    ncyl=1024;
    nsec=63;

    sbinit();
    // command
    tcp_server server = server_init(port, 1, on_connection, on_recv, cleanup);
    server_run(server);

    // never reached
    log_close();
}
