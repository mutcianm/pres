#include"util.h"

int main(int argc, char** argv){
    struct stream_t stream;
    pres_init(&stream, "res.txt", "wb+");
    pres_add(&stream, "1.txt");
    pres_add(&stream, "2.txt");
    pres_shutdown(&stream);
    pres_init(&stream, "res.txt", "rb");

    pres_shutdown(&stream);
    return 0;
}
