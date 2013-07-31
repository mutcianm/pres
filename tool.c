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
		" -g gilename\tglue first resource file from filelist to file filename\n"
		" -h\t\tprint this help message\n";

void helpexit(){
	printf("%s", helpstr);
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
				error(1, 0, "failed to init resource file");
			for(int j = i+2; j < argc; ++j){
				int ret = pres_add(&stream, argv[j]);
				if(ret != PRES_SUCESS)
					error(ret, 0, "failed adding resource");
			}
			pres_shutdown(&stream);
			break;
		} else {
			printf("wrong option %s\n", argv[i]);
			helpexit();
		}
	}
	return 0;
}
