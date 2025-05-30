
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "block.h"
#include "common.h"
#include "fs.h"
#include "log.h"

// global variables
int ncyl, nsec;

#define ReplyYes()       \
    do {                 \
        printf("Yes\n"); \
        Log("Success");  \
    } while (0)
#define ReplyNo(x)      \
    do {                \
        printf("No\n"); \
        Warn(x);        \
    } while (0)

// return a negative value to exit
int handle_f(char *args) {
    if (cmd_f(ncyl, nsec) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to format");
    }
    return 0;
}

int handle_mk(char *args) {
    char *name=strtok(args, " ");
    if (name == NULL) {
        ReplyNo("Missing file name");
        return 0;
    }
    char* m=strtok(NULL, " ");
    if (m == NULL) {
        ReplyNo("Missing mode name");
        return 0;
    }
    short mode = atoi(m);
    if (cmd_mk(name, mode) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to create file");
    }
    return 0;
}

int handle_mkdir(char *args) {
    char *name=strtok(args, " ");
    if (name == NULL) {
        ReplyNo("Missing dir name");
        return 0;
    }
    char* m=strtok(NULL, " ");
    if (m == NULL) {
        ReplyNo("Missing mode name");
        return 0;
    }
    short mode = atoi(m);
    if (cmd_mkdir(name, mode) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to create file");
    }
    return 0;
}

int handle_rm(char *args) {
    char *name=args;
    if (name == NULL) {
        ReplyNo("Missing file name");
        return 0;
    }
    if (cmd_rm(name) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to remove file");
    }
    return 0;
}

int handle_cd(char *args) {
    char *name=args;
    if (name == NULL) {
        ReplyNo("Missing dir name");
        return 0;
    }
    if (cmd_cd(name) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to change directory");
    }
    return 0;
}

int handle_rmdir(char *args) {
    char *name=args;
    if (name == NULL) {
        ReplyNo("Missing dir name");
        return 0;
    }
    if (cmd_rmdir(name) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to remove directory");
    }
    return 0;
}

int handle_ls(char *args) {
    entry *entries = NULL;
    int n = 0;
    if (cmd_ls(&entries, &n) != E_SUCCESS) {
        ReplyNo("Failed to list files");
        return 0;
    }
    ReplyYes();
    for(int i=0;i<n;i++){
        char* name=entries[i].name;
        uint type=entries[i].type;
        uint size=entries[i].size;
        uint update_time=entries[i].update_time;
        // uint user=entries[i].user;
        if(type==T_DIR){
            printf("name: %s type: Dir updated_time: %d \n",name,update_time);
            // printf("name: %s type: Dir updated_time: %d user: %d\n",name,update_time,user);
        }
        else{
            printf("name: %s type: File size: %d updated_time: %d \n",name,size,update_time);
            // printf("name: %s type: File size: %d updated_time: user: %d \n",name,size,update_time,user);
        }
    }
    free(entries);
    return 0;
}

int handle_cat(char *args) {
    char *name=args;
    if (name == NULL) {
        ReplyNo("Missing file name");
        return 0;
    }
    uchar *buf = NULL;
    uint len;
    if (cmd_cat(name, &buf, &len) == E_SUCCESS) {
        ReplyYes();
        uchar* tmp=malloc(sizeof(uchar)*(len+1));
        for(int i=0;i<len;i++){
            tmp[i]=buf[i];
        }
        tmp[len]='\0';
        printf("%s\n",tmp);
        
        free(tmp);
        free(buf);
    } else {
        ReplyNo("Failed to read file");
    }
    return 0;
}

int handle_w(char *args) {
    char *name;
    uint len;
    char *data;
    name = strtok(args, " ");
    if (name == NULL) {
        ReplyNo("Missing file name");
        return 0;
    }
    char *len_str = strtok(NULL, " ");
    if (len_str == NULL) {
        ReplyNo("Missing length");
        return 0;
    }
    data = strtok(NULL, " ");
    if (data == NULL) {
        ReplyNo("Missing data");
        return 0;
    }
    len = atoi(len_str);
    if (cmd_w(name, len, data) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to write file");
    }
    return 0;
}

int handle_i(char *args) {
    char *name;
    uint pos;
    uint len;
    char *data;
    name=strtok(args, " ");
    if (name == NULL) {
        ReplyNo("Missing file name");
        return 0;
    }
    char* pos_str=strtok(NULL, " ");
    if (pos_str == NULL) {
        ReplyNo("Missing position");
        return 0;
    }
    char* len_str=strtok(NULL, " ");
    if (len_str == NULL) {
        ReplyNo("Missing length");
        return 0;
    }
    data = strtok(NULL, " ");
    if (data == NULL) {
        ReplyNo("Missing data");
        return 0;
    }
    pos=atoi(pos_str);
    len=atoi(len_str);
    if (cmd_i(name, pos, len, data) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to write file");
    }
    return 0;
}

int handle_d(char *args) {
    char *name;
    uint pos;
    uint len;
    name=strtok(args, " ");
    if (name == NULL) {
        ReplyNo("Missing file name");
        return 0;
    }
    char* pos_str=strtok(NULL, " ");
    if (pos_str == NULL) {
        ReplyNo("Missing position");
        return 0;
    }
    char* len_str=strtok(NULL, " ");
    if (len_str == NULL) {
        ReplyNo("Missing length");
        return 0;
    }
    pos=atoi(pos_str);
    len=atoi(len_str);

    if (cmd_d(name, pos, len) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to delete file");
    }
    return 0;
}

int handle_e(char *args) {
    printf("Bye!\n");
    Log("Exit");
    return -1;
}

int handle_login(char *args) {
    int uid=atoi(args);
    if (cmd_login(uid) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to login");
    }
    return 0;
}

static struct {
    const char *name;
    int (*handler)(char *);
} cmd_table[] = {{"f", handle_f},        {"mk", handle_mk},       {"mkdir", handle_mkdir}, {"rm", handle_rm},
                 {"cd", handle_cd},      {"rmdir", handle_rmdir}, {"ls", handle_ls},       {"cat", handle_cat},
                 {"w", handle_w},        {"i", handle_i},         {"d", handle_d},         {"e", handle_e},
                 {"login", handle_login}};

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

FILE *log_file;

int main(int argc, char *argv[]) {
    log_init("fs.log");
    assert(BSIZE % sizeof(dinode) == 0);

    // get disk info and store in global variables
    // get_disk_info(&ncyl, &nsec);
    ncyl=1024;
    nsec=63;
    // read the superblock
    sbinit();

    static char buf[4096];
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (feof(stdin)) break;
        buf[strlen(buf) - 1] = 0;
        Log("Use command: %s", buf);
        char *p = strtok(buf, " ");
        int ret = 1;
        for (int i = 0; i < NCMD; i++)
            if (p && strcmp(p, cmd_table[i].name) == 0) {
                ret = cmd_table[i].handler(p + strlen(p) + 1);
                break;
            }
        if (ret == 1) {
            Log("No such command");
            printf("No\n");
        }
        if (ret < 0) break;
    }

    log_close();
}
