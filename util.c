#include"util.h"
#include<arpa/inet.h>

#define TABLE_INIT_SIZE 8
#define BUF_SIZE 1024

void head_init(struct header_t* head){
    head->dict = (char**)malloc(sizeof(char*)*TABLE_INIT_SIZE);
    head->sizes = (unsigned int*)malloc(sizeof(unsigned int)*TABLE_INIT_SIZE);
    head->offsets = (unsigned int*)malloc(sizeof(unsigned int)*TABLE_INIT_SIZE);
    head->allocated = TABLE_INIT_SIZE;
    head->dictsize = 0;
    head->pos = 0;
    head->totalsize = 0;
}

int head_grow(struct header_t* head){
    unsigned int newsize = head->dictsize * 2;
    head->dict = (char**)realloc(head->dict, newsize);
    head->sizes = (unsigned int*)realloc(head->sizes, newsize);
    head->offsets = (unsigned int*)realloc(head->offsets, newsize);
    if(!head->dict || !head->sizes || !head->offsets){
#ifdef DEBUG
        perror("Failed to grow header");
#endif
        return PRES_MEM_ERROR;
    }
    head->allocated = newsize;
    return PRES_SUCESS;
}

int head_add(struct header_t* head, char* str, unsigned int offset, unsigned int size){
    if(head->dictsize == head->allocated)
        if(head_grow(head) == PRES_MEM_ERROR)
            return PRES_MEM_ERROR;
    unsigned int pos = head->pos;
    head->dict[pos] = str;
    head->sizes[pos] = size;
    head->offsets[pos] = offset;
    head->dictsize++;
    head->totalsize += size;
    head->pos++;
    return PRES_SUCESS;
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
        head->offsets[i] = htonl(head->offsets[i]);
        head_len += sizeof(head->sizes[i])*fwrite(&head->offsets[i], sizeof(head->sizes[i]), 1, f);
    }
    head_len += sizeof(head->dictsize) + sizeof(head->totalsize) + sizeof(head_len) + sizeof(MAGICK);
    head->totalsize += head_len;
//    printf("%X %X %X\n", head->dictsize, head->totalsize, head_len);
    head->dictsize = htonl(head->dictsize);
    head->totalsize = htonl(head->totalsize);
    head_len = htonl(head_len);
    fwrite(&head->dictsize, sizeof(head->dictsize), 1, f);
    fwrite(&head->totalsize, sizeof(head->dictsize), 1, f);
    fwrite(&head_len, sizeof(head_len), 1, f);
    int magick = MAGICK;
    fwrite(&magick, sizeof(magick), 1, f);
    return PRES_SUCESS;
}

int head_read(FILE* f, struct header_t* head){
    fseek(f, -(sizeof(int)), SEEK_END);
    int magick_read;
    fread(&magick_read, sizeof(int), 1, f);
    if(magick_read != MAGICK){
        printf("magick number mismatch %X\n", magick_read);
        return PRES_BADMAGICK;
    }
    unsigned int head_len, totalsize, dictsize;
    fseek(f, -4*sizeof(unsigned int), SEEK_END);
    fread(&dictsize, sizeof(head->dictsize), 1, f);
    fread(&totalsize, sizeof(head->totalsize), 1, f);
    fread(&head_len, sizeof(head_len), 1, f);
    dictsize = ntohl(dictsize);
    totalsize = ntohl(totalsize);
    head_len = ntohl(head_len);
    fseek(f, -(int)head_len, SEEK_END);           //rewind to header start
//    printf("%X %X %X\n", dictsize, totalsize, head_len);
    for(int i = 0; i < dictsize; ++i){
        unsigned int keylen, size, offset;
        fread(&keylen, sizeof(keylen), 1, f);
        keylen = ntohl(keylen);
        char* key = (char*)malloc(keylen);
        fread(key, sizeof(char), keylen, f);
        fread(&size, sizeof(size), 1, f);
        fread(&offset, sizeof(offset), 1, f);
        size = ntohl(size);
        offset = ntohl(offset);
        head_add(head, key, offset, size);
    }
    head->totalsize = totalsize;
    return PRES_SUCESS;
}

int pres_init(struct stream_t* stream, const char* outfilename, const char* mode){
    if(!strcmp(mode, "wb+")){
        stream->mode = P_MODE_WRITE;
    } else if (!strcmp(mode, "rb")) {
        stream->mode = P_MODE_READ;
    } else {
        stream->mode = P_MODE_BAD;
        return PRES_BAD_MODE;
    }
    stream->file = fopen(outfilename, mode);
    if(!stream->file) return PRES_FILE_ERR;
    head_init(&stream->header);
    if(stream->mode == P_MODE_READ)
        return head_read(stream->file, &stream->header);
    return PRES_SUCESS;
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
    return PRES_SUCESS;

}
int pres_strip(const char* target){
	FILE* f = fopen(target, "rb");
    fseek(f, -(sizeof(int)), SEEK_END);
    int magick_read;
    fread(&magick_read, sizeof(int), 1, f);
    if(magick_read != MAGICK){
        printf("magick number mismatch %X\n", magick_read);
        return PRES_BADMAGICK;
    }
    unsigned int head_len, totalsize, dictsize;
    fseek(f, -4*sizeof(unsigned int), SEEK_END);
    fread(&dictsize, sizeof(dictsize), 1, f);
    fread(&totalsize, sizeof(totalsize), 1, f);
    fread(&head_len, sizeof(head_len), 1, f);
    dictsize = ntohl(dictsize);
    totalsize = ntohl(totalsize);
    head_len = ntohl(head_len);
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fclose(f);
    return truncate(target, fsize-totalsize);
}

int pres_add(struct stream_t* stream, char* resname){
    if(stream->mode != P_MODE_WRITE)
        return PRES_BAD_MODE;
    FILE* src = fopen(resname, "rb");
    if(!src){
#ifdef DEBUG
        perror("Failed to open src file");
#endif
        return PRES_FILE_ERR;
    }
    unsigned int newoffset;
    fseek(src, 0, SEEK_END);
    unsigned int fsize = ftell(src);
    fseek(src, 0, SEEK_SET);
    if(stream->header.pos == 0)
        newoffset = 0;
    else
        newoffset = stream->header.sizes[stream->header.pos-1] + stream->header.offsets[stream->header.pos-1];
    head_add(&stream->header, resname, newoffset, fsize);
    char buffer[BUF_SIZE];
    int numread = 0;
    while((numread = fread(buffer, sizeof(char), BUF_SIZE, src)) != 0){
        if(fwrite(buffer, sizeof(char), numread, stream->file) != numread){
#ifdef DEBUG
            perror("Failed to write to res file");
#endif
            return PRES_FILE_ERR;
        }
    }
    return PRES_SUCESS;
}

int pres_getsize(struct stream_t* stream, char* key){
	if((stream->mode == P_MODE_BAD) || (stream->mode == P_MODE_WRITE))
		return -1;
	int i = head_find(&stream->header, key);
	if(i < 0)
		return -1;
	return stream->header.sizes[i];
}

int pres_read(struct stream_t* stream, char* resname, char* buf, unsigned int num){
	if((stream->mode == P_MODE_BAD) || (stream->mode == P_MODE_WRITE))
		return PRES_FILE_ERR;
	int i = head_find(&stream->header, resname);
	if(i < 0)
		return PRES_NOKEY;
	unsigned int offset, size;
	offset = stream->header.offsets[i];
	size = stream->header.sizes[i];
	fseek(stream->file, -(int)(stream->header.totalsize - offset), SEEK_END);
	int numread = fread(buf, sizeof(char), num, stream->file);
	if(numread < num) return PRES_FILE_ERR;
	return PRES_SUCESS;
}

char* pres_read1(struct stream_t* stream, char* resname){
	if((stream->mode == P_MODE_BAD) || (stream->mode == P_MODE_WRITE))
		return NULL;
	int i = head_find(&stream->header, resname);
	if(i < 0)
		return NULL;
	unsigned int offset, size;
	offset = stream->header.offsets[i];
	size = stream->header.sizes[i];
	char* result = (char*)malloc(size);
	fseek(stream->file, -(int)(stream->header.totalsize - offset), SEEK_END);
	int numread = fread(result, sizeof(char), size, stream->file);
	if(numread <= 0){
		free(result);
		return NULL;
	}
	return result;
}

int pres_shutdown(struct stream_t* stream){
    if(stream->mode == P_MODE_WRITE) head_write(stream);
    fclose(stream->file);
    if((stream->mode == P_MODE_WRITE) || (stream->mode == P_MODE_BAD)) return PRES_SUCESS;    //we don't allocate strings in write mode
    int i;
    for(i = 0; i < stream->header.dictsize; i++){
        free(stream->header.dict[i]);
    }
    return PRES_SUCESS;
}

