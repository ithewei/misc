#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

size_t strlen(const char* s) {
    assert(s != NULL);
    size_t len = 0;
    while (*s++) ++len;
    return len;
}

char* strcpy(char* dst, const char* src) {
    assert(dst != NULL && src != NULL);
    char* ret = dst;
    while (*dst++ = *src++) {}
    return ret;
}

char* strcat(char* dst, const char* src) {
    assert(dst != NULL && src != NULL);
    char* ret = dst;
    while (*dst) {++dst;}
    while (*dst++ = *src++) {}
    return ret;
}

int strcmp(const char* s1, const char* s2) {
    assert(s1 != NULL && s2 != NULL);
    while (*s1 && *s2 && *s1 == *s2) {++s1; ++s2;}
    return *s1 - *s2;
}

int strncmp(const char* s1, const char* s2, int n) {
    assert(s1 != NULL && s2 != NULL);
    while (n > 0 && *s1 && *s2 && *s1 == *s2) {--n;++s1; ++s2;}
    return *s1 - *s2;
}

char* strrev(char* s) {
    if (s == NULL) return NULL;
    char* b = s;
    char* e = s;
    while(*e) {++e;}
    --e;
    char tmp;
    while (e > b) {
        tmp = *e;
        *e = *b;
        *b = tmp;
        --e;
        ++b;
    }
    return s;
}

char* strchr(const char* s, int c) {
    assert(s != NULL);
    while (*s && *s++ != c) {}
    return (char*)(*s == c ? s : NULL);
}

char* strrchr(const char* s, int c) {
    assert(s != NULL);
    const char* b = s;
    while (*s++) {}
    --s;
    while (s > b && *s-- != c) {}
    return (char*)(*s == c ? s : NULL);
}

char* strstr(const char* s, const char* find) {
    assert(s != NULL && find != NULL);
    int slen = 0;
    int flen = 0;
    const char* sp = s;
    const char* fp = find;
    while (*sp++) {++slen;}
    while (*fp++) {++flen;}
    int i;
    int n;
    for (i = 0; i <= slen - flen; ++i) {
        sp = s + i;
        fp = find;
        n = flen;
        while (n > 0 && *sp++ == *fp++) {--n;}
        if (n == 0) return (char*)(s+i);
    }
    return NULL;
}

char* strdup(const char* s) {
    assert(s != NULL);
    char* ret = (char*)malloc(strlen(s) + 1);
    if (ret == NULL) return NULL;
    strcpy(ret, s);
    return ret;
}

#include <stdio.h>
int main(int argc, char* argv[]) {
    char* s1 = argv[1];
    char* s2 = argv[2];
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    int cmp = strcmp(s1, s2);
    char* find = strstr(s1, s2);
    printf("s1=%s s=%s\n", s1, s2);
    printf("strlen1=%lu strlen2=%lu strcmp=%d strstr=%s\n",
        len1, len2, cmp, find);
    char* dup = strdup(s1);
    printf("strdup=%s\n", dup);
    char* rev = strrev(dup);
    printf("strrev=%s\n", rev);
    return 0;
}
