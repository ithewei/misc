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

#define SERVER_PORT     12007
#define MAX_BUFLEN      8192  // 8k

inline int perrno(const char* msg) {
    int errcode = errno;
    printf("%s: %s: %d\n", msg, strerror(errcode), errcode);
    return errcode;
}

inline void pexit(int exitcode) {
    printf("exit with code %d\n", exitcode);
    exit(exitcode);
}

int main(int argc, char** argv) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        pexit(perrno("WSAStartup"));
    }
#endif
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        pexit(perrno("socket"));
    }

    int ret = 0;

    int so_broadcast = 1;
    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast));
    if (ret < 0) {
        pexit(perrno("setsockopt SO_BROADCAST"));
    }

    struct sockaddr_in srvaddr;
    int addrlen = sizeof(srvaddr);
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(SERVER_PORT);
    srvaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    int buflen = MAX_BUFLEN;
    char* buf = (char*)malloc(buflen);
    if (buf == NULL)  {
        pexit(perrno("malloc"));
    }
    struct sockaddr_in srcaddr;
    socklen_t srcaddrlen = sizeof(srcaddr);

    memset(buf, 0, buflen);
    strcpy(buf, "GET /device_info");
    int sendbytes = sendto(sock, buf, strlen(buf), 0, (struct sockaddr*)&srvaddr, addrlen);
    if (sendbytes < 0) {
        pexit(perrno("sendto"));
    }
    printf("broadcast: %s\n", buf);

    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int device_num = 0;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        int num = select(sock+1, &readfds, NULL, NULL, &timeout);
        if (num < 0) {
            pexit(perrno("select"));
        }
        if (num == 0) {
            printf("timeout\n");
            break;
        }
        srcaddrlen = sizeof(srcaddr);
        memset(&srcaddr, 0, sizeof(srcaddr));
        memset(buf, 0, buflen);
        int recvbytes = recvfrom(sock, buf, buflen, 0, (struct sockaddr*)&srcaddr, &srcaddrlen);
        if (recvbytes < 0) {
            pexit(perrno("recvfrom"));
        }
        printf("recvfrom[%s:%d]: %s\n", inet_ntoa(srcaddr.sin_addr), ntohs(srcaddr.sin_port), buf);
        ++device_num;
    }

    printf("device_num=%d\n", device_num);

    close(sock);

    return 0;
}
