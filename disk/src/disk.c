#include "disk.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "log.h"

// global variables
int _ncyl, _nsec, ttd;
int ccyl;//current cylinder
char* diskfile=NULL;
int fd;
long FILESIZE;
int init_disk(char *filename, int ncyl, int nsec, int ttd) {
    _ncyl = ncyl;
    _nsec = nsec;
    ttd = ttd;
    ccyl=0;
    // do some initialization...
    fd = open(filename, O_RDWR | O_CREAT, 0644);
    if(fd < 0){
        fprintf(stderr, "fail to open the file%s", filename);
        exit(EXIT_FAILURE);
    }
    // open file
    FILESIZE= BSIZE * _ncyl * _nsec;
    int result = lseek(fd, FILESIZE-1, SEEK_SET);
    if(result < 0){
        fprintf(stderr, "fail to call lseek()");
        close(fd);
        exit(EXIT_FAILURE);
    }
    // stretch the file
    result = write(fd, "", 1);
    if(result < 0){
        fprintf(stderr, "fail to write the last byte of the file");
        close(fd);
        exit(EXIT_FAILURE);
    }
    diskfile =  (char *)mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(diskfile == MAP_FAILED){
        close(fd);
        fprintf(stderr, "fail to map the file");
        exit(EXIT_FAILURE);
    }
    // mmap

    Log("Disk initialized: %s, %d Cylinders, %d Sectors per cylinder", filename, ncyl, nsec);
    return 0;
}

// all cmd functions return 0 on success
int cmd_i(int *ncyl, int *nsec) {
    // get the disk info
    *ncyl = _ncyl;
    *nsec = _nsec;
    return 0;
}

int cmd_r(int cyl, int sec, char *buf) {
    // read data from disk, store it in buf
    if (cyl >= _ncyl || sec >= _nsec || cyl < 0 || sec < 0) {
        Log("Invalid cylinder or sector");
        return 1;
    }
    int delay = ttd * abs(cyl-ccyl);
    ccyl=cyl;
    usleep(delay);
    //move the arm
    memcpy(buf, &diskfile[BSIZE * (ccyl * _nsec + sec)], BSIZE);
    //read the block
    if(buf == NULL){
        Log("fail to read the block: %d Cylinder, %d Sector", cyl, sec);
        return 1;
    }
    Log("Read the block: %d Cylinder, %d Sector", cyl, sec);
    return 0;
}

int cmd_w(int cyl, int sec, int len, char *data) {
    if (cyl >= _ncyl || sec >= _nsec || cyl < 0 || sec < 0 || len > BSIZE || len < 0) {
        Log("Invalid cylinder or sector: %d Cylinder, %d Sector, %d byte(s). The data: %s", cyl, sec, len, data);
        return 1;
    }

    int delay = ttd * abs(cyl-ccyl);
    ccyl=cyl;
    usleep(delay);
    //move the arm
    memcpy(&diskfile[BSIZE * (ccyl * _nsec + sec)],data,len);
    // write data to disk
    Log("Write to the block: %d Cylinder, %d Sector, %d byte(s). The data: %s", cyl, sec, len, data);
    return 0;
}

void close_disk() {
    // close the file
    close(fd);
    Log("Close the disk");
}
