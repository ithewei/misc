#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __unix__
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>

typedef int socklen_t;
#endif

#define BIND_PORT   12007
#define MAX_BUFLEN  8192  // 8k

int perrno(const char* msg) {
    int errcode = errno;
    printf("%s: %s: %d\n", msg, strerror(errcode), errcode);
    return errcode;
}

void pexit(int exitcode) {
    printf("exit with code %d\n", exitcode);
    exit(exitcode);
}

int on_get_device_info(char* buf, int buflen) {
    memset(buf, 0, buflen);
    strcat(buf, "name=camera007&");
    strcat(buf, "type=camera&");
    strcat(buf, "version=1.0.0.0");
    return 0;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        pexit(perrno("WSAStartup"));
    }
#endif
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        pexit(perrno("socket"));
    }

    struct sockaddr_in srvaddr;
    int addrlen = sizeof(srvaddr);
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(BIND_PORT);
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(sock, (struct sockaddr*)&srvaddr, addrlen);
    if (ret < 0) {
        pexit(perrno("bind"));
    }

    int buflen = MAX_BUFLEN;
    char* buf = (char*)malloc(buflen);
    if (buf == NULL)  {
        pexit(perrno("malloc"));
    }
    struct sockaddr_in srcaddr;
    socklen_t srcaddrlen = sizeof(srcaddr);

#ifdef __unix__
    ret = daemon(1, 1);
    if (ret < 0) {
        pexit(perrno("daemon"));
    }
#endif

    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    int exit_loop = 0;
    while (!exit_loop) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        int num = select(sock+1, &readfds, NULL, NULL, &timeout);
        if (num < 0) {
            perrno("select");
            continue;
        }
        if (num == 0) {
            continue;
        }
        srcaddrlen = sizeof(srcaddr);
        memset(&srcaddr, 0, sizeof(srcaddr));
        memset(buf, 0, buflen);
        int recvbytes = recvfrom(sock, buf, buflen, 0, (struct sockaddr*)&srcaddr, &srcaddrlen);
        if (recvbytes < 0) {
            perrno("recvfrom");
            continue;
        }
        printf("recvfrom[%s:%d]: %s\n", inet_ntoa(srcaddr.sin_addr), ntohs(srcaddr.sin_port), buf);
        if (strcmp(buf, "GET /device_info") == 0) {
            on_get_device_info(buf, buflen);
            int sendbytes = sendto(sock, buf, strlen(buf), 0, (struct sockaddr*)&srcaddr, srcaddrlen);
            if (sendbytes < 0) {
                perrno("sendto");
                continue;
            }
            printf("sendto: %s\n", buf);
        }
    }

    if (buf) {
        free(buf);
    }
}
