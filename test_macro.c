#include <stdio.h>

int main(int argc, char** argv) {
#ifdef _WIN32_WINNT
    printf("#define _WIN32_WINNT %d\n", _WIN32_WINNT);
#endif

#ifdef _MSC_VER
    printf("#define _MSC_VER %d\n", _MSC_VER);
#endif

#ifdef __GNUC__
    printf("#define __GNUC__ %d\n", __GNUC__);
#endif

#ifdef DEBUG
    puts("DEBUG");
#endif

#ifdef _DEBUG
    puts("_DEBUG");
#endif

#ifdef NODEBUG
    puts("NODEBUG");
#endif

#ifdef UNICODE
    puts("UNICODE");
#endif

#ifdef _UNICODE
    puts("_UNICODE");
#endif

#ifdef WIN32
    puts("WIN32");
#endif

#ifdef WIN64
    puts("WIN64");
#endif

#ifdef _WIN32
    puts("_WIN32");
#endif

#ifdef _WIN64
    puts("_WIN64");
#endif

#ifdef unix
    puts("unix");
#endif

#ifdef linux
    puts("linux");
#endif

#ifdef __unix__
    puts("__unix__");
#endif

#ifdef __linux__
    puts("__linux__");
#endif

    return 0;
}
