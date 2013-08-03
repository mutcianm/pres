#ifndef PRES_UTIL_H
#define PRES_UTIL_H
/**
 *  @file util.h
 *  @brief pres api
 */


#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#define MAGICK  0xDEAFCAB1

#define P_MODE_READ     0
#define P_MODE_WRITE    1
#define P_MODE_BAD      -1

#define PRES_SUCCESS     0
#define PRES_EXISTS     1
#define PRES_FILE_ERR   2
#define PRES_BAD_MODE   3
#define PRES_MEM_ERROR  4
#define PRES_BADMAGICK  5
#define PRES_NOKEY		6

/**
 * Stores info about available resources. success 
 * User normally won't need this, unless for curiosity reasons.
 * @warning Do not modify this data manually, else it would lead to undefined behavior.
 */
struct header_t{
    char**  dict;             //!<array of string keys
    unsigned int* offsets;    //!<array of resource offsets from the beginning of resource block
    unsigned int* sizes;      //!<array of compressed resource sizes
    unsigned int* unc_sizes;  //!<array of uncompressed resource sizes
    unsigned int  dictsize;   //!<number of entries
    unsigned int  totalsize;  //!<total size of resources block including header
    char			  level;  //!<compression level
    unsigned int  allocated;  //!<current size of allocated table
    unsigned int  pos;        //!<current operating position
};

/**
 * Stores info about resource file.
 * User normally won't need this, unless for curiosity reasons.
 * @warning Do not modify this data manually, else it would lead to undefined behavior.
 */
struct stream_t{
    FILE* file;
    struct header_t header;
    int mode;
    unsigned int* pos;
};

/**
 * Sets up pres. One must call this first to begin working with resources.
 * @param stream pointer to uninitialized structure, it will be properly set up by this function
 * @param outfilename the file containing resources, might as well be program's own executable(argv[0]), if mode is "wb+" file will be truncated.
 * @param mode same syntax as in fopen(), for reading you'd like "rb", for writing "ab+" or "wb+", "b" is important since pres operates in binary mode
 * @param complevel compression level, must be in range [0..9]; if opening for reading, can pass any value, the real one will be taken from file
 * @return PRES_SUCCESS on success, else respective error code. @note since most errors happen on system layer, errno can be used to get more info
 */
int pres_init(struct stream_t* stream, const char* outfilename, const char* mode, char complevel);

/**
 * Add file specified by resname to resource package specified by stream.
 * @note in order to have all resources in one pack call this between *one* init/shutdown call pair
 * @param stream must be initialized beforehand with pres_init()
 * @param resname file to be added as a resource
 * @return PRES_SUCCESS on success, PRES_BADMODE if stream has been opened for reading, else respective error code
 */
int pres_add(struct stream_t* stream, char* resname);

/**
 * Utility function used to append resource file to any other file.
 * In fact it will append any file to any other file.
 * @note this function doesn't need a stream pointer to operate, it handles raw files
 * @param src file to append to target
 * @param target file to which src will be appended
 * @return PRES_SUCCESS on success, else respective error code
 */
int pres_glue(const char* src, const char* target);

/**
 * Attempts to strip resources from file target.
 * @param target name of a file to strip resources from
 * @return PRES_SUCCESS on success, PRES_BADMAGICK if target doesn't have any resources attached or header couldn't be read
 */
int pres_strip(const char* target);

/**
 * Gets size of resource specified by key.
 * @param stream must be initialized beforehand with pres_init()
 * @param key resource name to look for
 * @return respective size of resource of -1 if no resource found for given key
 */
int pres_getsize(struct stream_t* stream, char* key);

/**
 * Reads num bytes of resource specified by resname from bound stream into buf.
 * @param stream resource stream to read from. Must be initialized beforehand with pres_init()
 * @param resname name of resource of key to read
 * @param buf must be at least num size
 * @param num number of bytes to read from resource @see pres_getsize()
 * @return PRES_SUCCESS on success, else respective error code
 * @warning if num is bigger than actual resource size, undefined behavior is to be expected
 */
int pres_read(struct stream_t* stream, char* resname, char* buf, unsigned int num);

/**
 * Simplified version of pres_read().
 * Allocates proper buffer, reads entire resource into it and returns the data.
 * @param stream resource stream to read from. Must be initialized beforehand with pres_init()
 * @param resname name of resource of key to read
 * @return pointer to buffer containing data if read was a success, NULL if some error happened
 * @warning buffer is allocated by this function with malloc(), disposing it is user's responsibility
 */
char* pres_read1(struct stream_t* stream, char* resname);

/**
 * Call this when finished working with resource file.
 * Its safe to call this function on already closed stream.
 * @param stream stream to close
 * @return PRES_SUCCESS on success, else respective error code
 */
int pres_shutdown(struct stream_t* stream);

#endif
