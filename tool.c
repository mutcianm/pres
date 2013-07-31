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
		"\tprestool [options] <input files ...>\n"
		"OPTIONS\n"
		" -a resfile\tadd list of input files to resource file resfile\n"
		" -g target res\tglue res resource file to file target\n"
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
	for(int i = 1; i < argc; ++i){
		if(!strcmp(argv[i], "-a")){
			if(i+1 == argc) helpexit();
			char* out = argv[i+1];
			struct stream_t stream;
			if(pres_init(&stream, out, "wb+") != PRES_SUCESS)
				pexit("Failed initializing resource file");
			for(int j = i+2; j < argc; ++j){
				int ret = pres_add(&stream, argv[j]);
				if(ret != PRES_SUCESS)
					pexit("failed adding resource");
			}
			pres_shutdown(&stream);
			break;
		} else if(!strcmp(argv[i], "-g")){
			if(i+2 == argc) helpexit();
			int ret = pres_glue(argv[i+2], argv[i+1]);
			if(ret != PRES_SUCESS)
				pexit("failed to glue");
			break;
		} else if(!strcmp(argv[i], "-s")){
			if(i+1 == argc) helpexit();
			if(pres_strip(argv[i+1]) != PRES_SUCESS)
				pexit("failed sripping resources from file");
			break;
		} else if(!strcmp(argv[i], "-h")){
			printf("%s", helpstr);
			break;
		} else if(!strcmp(argv[i], "-i")){
			if(i+1 == argc) helpexit();
			struct stream_t stream;
			if(pres_init(&stream, argv[i+1], "rb") != PRES_SUCESS)
				pexit("Failed initializing resource file");
			struct header_t* head = &stream.header;
			printf("entries:%d totalsize:%d\n", head->dictsize, head->totalsize);
			for(int j = 0; j < head->dictsize; ++j){
				printf("%s: size=%d offset=%d\n", head->dict[j], head->sizes[j], head->offsets[j]);
			}
			break;
		} else if(!strcmp(argv[i], "-r")){
			if(i+2 == argc) helpexit();
			struct stream_t stream;
			if(pres_init(&stream, argv[i+1], "rb") != PRES_SUCESS)
				pexit("Failed initializing resource file");
			char* buf = pres_read1(&stream, argv[i+2]);
			if(!buf)
				pexit("key not found");
			else
				puts(buf);
			break;
		} else {
			printf("Unrecognized option %s\n", argv[i]);
			helpexit();
		}
	}
	return 0;
}