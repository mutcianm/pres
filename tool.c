/*
 * tool.c
 *
 *  Created on: Jul 31, 2013
 *      Author: miha
 */

#include"util.h"
#include<error.h>

char* helpstr = "prestool - pres manipulation utility\n"
		"USAGE\n"
		"\tprestool [options] <action> <input files ...>\n\n"
		"OPTIONS\n"
		" -c NUM\t\tset compression level, must be in 0-9 range\n\n"
		"ACTIONS\n"
		" -a resfile\tadd list of input files to resource file resfile\n"
		" -g target res\tglue existing res resource file to file target\n"
		" -s target\tstrip resources from file target\n"
		" -i resfile\tprint res file content summary\n"
		" -r res key\tread specified key from file res to stdout\n"
		" -h\t\tprint this help message\n";

void helpexit(){
	printf("%s", helpstr);
	exit(1);
}

void pexit(const char* mess){
	perror(mess);
	exit(1);
}

int main(int argc, char** argv){
	if(argc == 1) helpexit();
	int i;
	unsigned level = 0;
	for(i = 1; i < argc; ++i){
		if(!strcmp(argv[i], "-a")){
			if(i+1 == argc) helpexit();
			char* out = argv[i+1];
			struct stream_t stream;
			if(pres_init(&stream, out, "ab+", level) != PRES_SUCCESS)
				pexit("Failed initializing resource file");
			int j;
			for(j = i+2; j < argc; ++j){
				int ret = pres_add(&stream, argv[j]);
				if(ret != PRES_SUCCESS)
					pexit("failed adding resource");
			}
			pres_shutdown(&stream);
			break;
		} else if(!strcmp(argv[i], "-g")){
			if(i+2 == argc) helpexit();
			int ret = pres_glue(argv[i+2], argv[i+1]);
			if(ret != PRES_SUCCESS)
				pexit("failed to glue");
			break;
		} else if(!strcmp(argv[i], "-s")){
			if(i+1 == argc) helpexit();
			if(pres_strip(argv[i+1]) != PRES_SUCCESS)
				pexit("failed sripping resources from file");
			break;
		} else if(!strcmp(argv[i], "-h")){
			printf("%s", helpstr);
			break;
		} else if(!strcmp(argv[i], "-i")){
			if(i+1 == argc) helpexit();
			struct stream_t stream;
			switch (pres_init(&stream, argv[i+1], "rb", level)){
				case PRES_BADMAGICK:
					error(1, 0, "Magic number not found! File is corrupt or not a resfile at all");
					/* no break */
				case PRES_FILE_ERR:
					pexit("File IO error");
					/* no break */
				case PRES_SUCCESS:
					break;
				default:
					pexit("Unknown error happened");
					/* no break */
			}
			struct header_t* head = &stream.header;
			printf("Entries:%d Totalsize:%d Compression level:%d\n", head->dictsize, head->totalsize, head->level);
			printf("%-10s %10s %10s %10s\n", "Name", "Size", "Uncomp", "Offset");
			int j;
			for(j = 0; j < head->dictsize; ++j){
				printf("%-10s  %10d  %10d  %10d\n", head->dict[j],
								head->sizes[j],
								head->unc_sizes[j],
								head->offsets[j]);
			}
			pres_shutdown(&stream);
			break;
		} else if(!strcmp(argv[i], "-r")){
			if(i+2 == argc) helpexit();
			struct stream_t stream;
			if(pres_init(&stream, argv[i+1], "rb", level) != PRES_SUCCESS)
				pexit("Failed initializing resource file");
			unsigned size = pres_getsize(&stream, argv[i+2]);
			char* buf = pres_read1(&stream, argv[i+2]);
			pres_shutdown(&stream);
			if(!buf){
				pexit("key not found");
			}else {
				fwrite(buf, sizeof(char), size, stdout);
				fflush(stdout);
				free(buf);
			}
			break;
		} else if(!strcmp(argv[i], "-c")){
			if(i+1 == argc) helpexit();
			level = atoi(argv[i+1]);
			if(level < 0 || level > 9){
				printf("Compression level must be in range 0-9\n");
				exit(1);
			}
			i++;
		}else {
			printf("Unrecognized argument %s\n", argv[i]);
			helpexit();
		}
	}
	return 0;
}
