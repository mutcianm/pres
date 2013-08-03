#include"util.h"
#include<arpa/inet.h>
#include<zlib.h>
#include<assert.h>

#define TABLE_INIT_SIZE 8
#define BUF_SIZE 4*1024
#define CHUNK 16*1024

//TODO: better error handling

void head_init(struct header_t* head, char complevel){
    head->dict = (char**)malloc(sizeof(char**)*TABLE_INIT_SIZE);
    head->sizes = (unsigned int*)malloc(sizeof(unsigned int)*TABLE_INIT_SIZE);
    head->unc_sizes = (unsigned int*)malloc(sizeof(unsigned int)*TABLE_INIT_SIZE);
    head->offsets = (unsigned int*)malloc(sizeof(unsigned int)*TABLE_INIT_SIZE);
    head->allocated = TABLE_INIT_SIZE;
    head->dictsize = 0;
    head->pos = 0;
    head->totalsize = 0;
    head->level = complevel;
}

int head_grow(struct header_t* head){
    unsigned int newsize = head->dictsize * 2;
    head->dict = (char**)realloc(head->dict, sizeof(char**)*newsize);
    head->sizes = (unsigned int*)realloc(head->sizes, sizeof(unsigned int)*newsize);
    head->unc_sizes = (unsigned int*)realloc(head->unc_sizes, sizeof(unsigned int)*newsize);
    head->offsets = (unsigned int*)realloc(head->offsets, sizeof(unsigned int)*newsize);
    if(!head->dict || !head->sizes || !head->offsets){
#ifdef DEBUG
        perror("Failed to grow header");
#endif
        return PRES_MEM_ERROR;
    }
    head->allocated = newsize;
    return PRES_SUCCESS;
}

int head_add(struct header_t* head, char* str, unsigned int offset, unsigned int size, unsigned unc_size){
    if(head->dictsize == head->allocated)
        if(head_grow(head) == PRES_MEM_ERROR)
            return PRES_MEM_ERROR;
    unsigned int pos = head->pos;
    head->dict[pos] = str;
    head->sizes[pos] = size;
    head->unc_sizes[pos] = unc_size;
    head->offsets[pos] = offset;
    head->dictsize++;
    head->totalsize += size;
    head->pos++;
    return PRES_SUCCESS;
}

int head_find(struct header_t* head, char* key){
	int i;
	for(i = 0; i < head->dictsize; ++i){
		if(!strcmp(head->dict[i], key))
			return i;
	}
	return -1;
}

int head_write(struct stream_t* stream){
    int i;
    struct header_t* head = &stream->header; 
    FILE* f = stream->file;
    unsigned int head_len = 0;
    for(i = 0; i < head->dictsize; ++i){
        unsigned int keylen = strlen(head->dict[i]) + 1;
        unsigned int keylen_f = htonl(keylen);
        head_len += sizeof(keylen_f)*fwrite(&keylen_f, sizeof(keylen_f), 1, f);
        head_len += fwrite(head->dict[i], sizeof(char), keylen, f);
        head->sizes[i] = htonl(head->sizes[i]);
        head_len += sizeof(head->sizes[i])*fwrite(&head->sizes[i], sizeof(head->sizes[i]), 1, f);
        head->unc_sizes[i] = htonl(head->unc_sizes[i]);
        head_len += sizeof(head->unc_sizes[i])*fwrite(&head->unc_sizes[i], sizeof(head->unc_sizes[i]), 1, f);
        head->offsets[i] = htonl(head->offsets[i]);
        head_len += sizeof(head->offsets[i])*fwrite(&head->offsets[i], sizeof(head->offsets[i]), 1, f);
    }
    head_len += sizeof(head->dictsize) + sizeof(head->totalsize) + sizeof(head_len) +
    		sizeof(head->level)+ sizeof(MAGICK);
    head->totalsize += head_len;
    head->dictsize = htonl(head->dictsize);
    head->totalsize = htonl(head->totalsize);
    head_len = htonl(head_len);
    fwrite(&head->dictsize, sizeof(head->dictsize), 1, f);
    fwrite(&head->totalsize, sizeof(head->dictsize), 1, f);
    fwrite(&head->level, sizeof(head->level), 1, f);
    fwrite(&head_len, sizeof(head_len), 1, f);
    int magick = MAGICK;
    fwrite(&magick, sizeof(magick), 1, f);
    return PRES_SUCCESS;
}

int head_read(FILE* f, struct header_t* head){
    fseek(f, -(sizeof(int)), SEEK_END);
    int magick_read;
    fread(&magick_read, sizeof(int), 1, f);
    if(magick_read != MAGICK)
        return PRES_BADMAGICK;
    unsigned int head_len, totalsize, dictsize;
    fseek(f, -4*sizeof(unsigned int)-1, SEEK_END);
    fread(&dictsize, sizeof(head->dictsize), 1, f);
    fread(&totalsize, sizeof(head->totalsize), 1, f);
    fread(&head->level, sizeof(head->level), 1, f);
    fread(&head_len, sizeof(head_len), 1, f);
    dictsize = ntohl(dictsize);
    totalsize = ntohl(totalsize);
    head_len = ntohl(head_len);
    fseek(f, -(int)head_len, SEEK_END);           //rewind to header start
    int i;
    for(i = 0; i < dictsize; ++i){
        unsigned int keylen, size, unc_size, offset;
        fread(&keylen, sizeof(keylen), 1, f);
        keylen = ntohl(keylen);
        char* key = (char*)malloc(keylen);
        fread(key, sizeof(char), keylen, f);
        fread(&size, sizeof(size), 1, f);
        fread(&unc_size, sizeof(size), 1, f);
        fread(&offset, sizeof(offset), 1, f);
        size = ntohl(size);
        offset = ntohl(offset);
        unc_size = ntohl(unc_size);
        head_add(head, key, offset, size, unc_size);
    }
    head->totalsize = totalsize;
    return PRES_SUCCESS;
}

int pres_init(struct stream_t* stream, const char* outfilename, const char* mode, char complevel){
    if(!strcmp(mode, "wb+") || !strcmp(mode, "ab") || !strcmp(mode, "ab+")){
        stream->mode = P_MODE_WRITE;
    } else if (!strcmp(mode, "rb")) {
        stream->mode = P_MODE_READ;
    } else {
        stream->mode = P_MODE_BAD;
        return PRES_BAD_MODE;
    }
    stream->file = fopen(outfilename, mode);
    if(!stream->file)
    	return PRES_FILE_ERR;
    head_init(&stream->header, complevel);
    if(stream->mode == P_MODE_READ)
        return head_read(stream->file, &stream->header);
    return PRES_SUCCESS;
}

int pres_glue(const char* src, const char* target){
    FILE* out = fopen(target, "ab+");
    FILE* in = fopen(src, "rb");
    if(!in || !out)
    	return PRES_FILE_ERR;
    char buffer[BUF_SIZE];
    unsigned int numread;
    while((numread = fread(buffer, sizeof(char), BUF_SIZE, in)) != 0){
        if(fwrite(buffer, sizeof(char), numread, out) != numread){
#ifdef DEBUG
            perror("Failed to write to res file");
#endif
            return PRES_FILE_ERR;
        }
    }
    return PRES_SUCCESS;

}
int pres_strip(const char* target){
	FILE* f = fopen(target, "rb");
	if(!f)
    	return PRES_FILE_ERR;
    fseek(f, -(sizeof(int)), SEEK_END);
    int magick_read;
    fread(&magick_read, sizeof(int), 1, f);
    if(magick_read != MAGICK)
        return PRES_BADMAGICK;
    unsigned int head_len, totalsize, dictsize;
    char level;
    fseek(f, -4*sizeof(unsigned int)-1, SEEK_END);
    fread(&dictsize, sizeof(dictsize), 1, f);
    fread(&totalsize, sizeof(totalsize), 1, f);
    fread(&level, sizeof(level), 1, f);
    fread(&head_len, sizeof(head_len), 1, f);
    dictsize = ntohl(dictsize);
    totalsize = ntohl(totalsize);
    head_len = ntohl(head_len);
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fclose(f);
    if(truncate(target, fsize-totalsize) != 0)
    	return PRES_FILE_ERR;
    return PRES_SUCCESS;
}

unsigned simple_add(FILE* src, FILE* dest){
	int numread = 0;
	unsigned totalwritten = 0;
	char buffer[BUF_SIZE];
    while((numread = fread(buffer, sizeof(char), BUF_SIZE, src)) != 0){
    	unsigned written = fwrite(buffer, sizeof(char), numread, dest);
        if(written != numread)
            return 0;
        totalwritten += written;
    }
    return totalwritten;
}

unsigned int do_add(FILE* src, FILE* dest, int level, unsigned* unc_size){
	if (level == 0){
		*unc_size = simple_add(src, dest);
		return *unc_size;
	}
	int ret, flush;
	unsigned have;
	unsigned totalwritten = 0;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];
    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return 0;
    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, src);
        if (ferror(src)) {
            (void)deflateEnd(&strm);
            return 0;
        }
        *unc_size += strm.avail_in;
        flush = feof(src) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;
        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            totalwritten += have;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return 0;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */
        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */
    /* clean up and return */
    (void)deflateEnd(&strm);
    return totalwritten;
}

int pres_add(struct stream_t* stream, char* resname){
    if(stream->mode != P_MODE_WRITE)
        return PRES_BAD_MODE;
    FILE* src = fopen(resname, "rb");
    if(!src)
        return PRES_FILE_ERR;
    int i = stream->header.pos - 1;
    unsigned newoffset = (stream->header.pos == 0) ?  0 : stream->header.sizes[i] + stream->header.offsets[i];
    unsigned unc_size = 0;
    unsigned size = do_add(src, stream->file, stream->header.level, &unc_size);
    if(size == 0)
    	return PRES_FILE_ERR;
    head_add(&stream->header, resname, newoffset, size, unc_size);
    return PRES_SUCCESS;
}

int pres_getsize(struct stream_t* stream, char* key){
	if((stream->mode == P_MODE_BAD) || (stream->mode == P_MODE_WRITE))
		return -1;
	int i = head_find(&stream->header, key);
	if(i < 0)
		return -1;
	return stream->header.unc_sizes[i];
}

unsigned simple_read(FILE *source, int offset, char* dest, unsigned size){
	fseek(source, -offset, SEEK_END);
	return fread(dest, sizeof(char), size, source);
}

unsigned do_read(FILE *source, int offset, char* dest, unsigned size, int level)
{
	if(level == 0)
		return simple_read(source, offset, dest, size);
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    unsigned buf_pos = 0;
    unsigned totalread = 0;
	fseek(source, -offset, SEEK_END);
    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return 0;
    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return 0;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;
        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
                /* no break */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return 0;
            }
            have = CHUNK - strm.avail_out;
            totalread += have;
            memcpy(dest + buf_pos, out, have);
            buf_pos += have;
        } while (strm.avail_out == 0);
        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);
    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? totalread : 0;
}

int pres_read(struct stream_t* stream, char* resname, char* buf, unsigned int num){
	if((stream->mode == P_MODE_BAD) || (stream->mode == P_MODE_WRITE))
		return PRES_FILE_ERR;
	int i = head_find(&stream->header, resname);
	if(i < 0)
		return PRES_NOKEY;
	unsigned int offset;
	offset = stream->header.offsets[i];
	int read_offset = (int)(stream->header.totalsize - offset);
	int numread = do_read(stream->file, read_offset, buf, num, stream->header.level);
	if(numread < num)
		return PRES_FILE_ERR;
	return PRES_SUCCESS;
}

char* pres_read1(struct stream_t* stream, char* resname){
	if((stream->mode == P_MODE_BAD) || (stream->mode == P_MODE_WRITE))
		return NULL;
	int i = head_find(&stream->header, resname);
	if(i < 0)
		return NULL;
	unsigned int offset, size;
	offset = stream->header.offsets[i];
	size = stream->header.unc_sizes[i];
	char* result = (char*)malloc(size);
	int read_offset = (int)(stream->header.totalsize - offset);
	int numread = do_read(stream->file, read_offset, result, size, stream->header.level);
	if(numread <= 0){
		free(result);
		return NULL;
	}
	return result;
}

int pres_shutdown(struct stream_t* stream){
	if(stream->mode == P_MODE_BAD || !stream->file) return PRES_SUCCESS;
    if(stream->mode == P_MODE_WRITE) head_write(stream);
    if(fclose(stream->file) != 0)
    	return PRES_FILE_ERR;
    stream->file = NULL;
    free(stream->header.offsets);
    free(stream->header.sizes);
    free(stream->header.unc_sizes);
    if(stream->mode == P_MODE_WRITE){
    	free(stream->header.dict);
    	return PRES_SUCCESS;    //we don't allocate strings in write mode
    }
    int i;
    for(i = 0; i < stream->header.dictsize; i++){
        free(stream->header.dict[i]);
    }
    free(stream->header.dict);
    return PRES_SUCCESS;
}

