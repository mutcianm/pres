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
		" -a filename\tadd list of input files to resource file filename\n"
		" -g filename\tglue first resource file from filelist to file filename\n"
		" -s filename\tstrip resources from file filename\n"
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
		} else {
			printf("Unrecognized option %s\n", argv[i]);
			helpexit();
		}
	}
	return 0;
}
