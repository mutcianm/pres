#include"util.h"

int main(int argc, char** argv){
	if(argc != 2){
		printf("usage: ./tester key\nappend resorces to tester with prestool first\n");
		return 1;
	}
    struct stream_t stream;
    pres_init(&stream, argv[0], "rb");
    char* str = pres_read1(&stream, argv[1]);
    if(str)
    	printf("%s\n", str);
    else
    	printf("no value foud for key %s\n", argv[1]);
    pres_shutdown(&stream);
    return 0;
}
