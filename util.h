#ifndef PRES_UTIL_H
#define PRES_UTIL_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define DEBUG

#define MAGICK  0xDEAFCAB1

#define P_MODE_READ     0
#define P_MODE_WRITE    1
#define P_MODE_BAD      -1

#define PRES_SUCESS     0
#define PRES_EXISTS     1
#define PRES_FILE_ERR   2
#define PRES_BAD_MODE   3
#define PRES_MEM_ERROR  4
#define PRES_BADMAGICK  5

struct header_t{
    char**  dict;       //array of string keys
    unsigned int* offsets;    //array of resorce offsets from the beginning of resource block
    unsigned int* sizes;      //array of resource sizes 
    unsigned int  dictsize;   //nuber of entries
    unsigned int  totalsize;  //total size of resorces block including header
    unsigned int  allocated;  //current size of allocated table
    unsigned int  pos;        //current operationg position
};

struct stream_t{
    char* filename;
    FILE* file;
    struct header_t header;
    int mode;
};

int pres_init(struct stream_t* stream, const char* outfilename, const char* mode);
int pres_shutdown(struct stream_t* stream);
int pres_glue(const char* src, const char* target);
int pres_strip(const char* target);

int pres_add(struct stream_t* stream, char* resname);
int pres_read(struct stream_t* stream, char* resname, char* buf, unsigned int num);

#endif
