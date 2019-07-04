#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SYS_NERR
#define SYS_NERR    133
#endif

void perrcode(int errcode) {
    const char* errmsg = strerror(errcode);
    if (errmsg == NULL) {
        errmsg = "Unknown error";
    }
    printf("%d: %s\n", errcode, errmsg);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: perrno [-l] [errno]\n");
        return -10;
    }

    if (strcmp(argv[1], "-l") == 0) {
        int i = 0;
        for (; i < SYS_NERR; ++i) {
            perrcode(i);
        }
    }
    else {
        int errcode = atoi(argv[1]);
        perrcode(errcode);
    }

    return 0;
}
