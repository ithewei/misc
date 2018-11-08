#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define EXEC_FAIL_EXITCODE  110

void pexit(int code) {
    printf("Exit with code %d.\n", code);
    exit(code);
}

void perrno() {
    int code = errno;
    printf("%s: %d\n", strerror(code), code);
}

void perrno_exit() {
    int code = errno;
    printf("%s\n", strerror(code));
    printf("Exit with code %d.\n", code);
    exit(code);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        puts("Usage: protected program argv...");
        pexit(-10);
    }

    // [0, argc-1]
    int len = sizeof(argc)*sizeof(char*);
    char** child_argv = (char**)malloc(len);
    memset(child_argv, 0, len);

    // [1, argc-1] => [0, argc-2]
    int i = 1, j = 0;
    for (; i < argc; ++i, ++j) {
        int len = strlen(argv[i]) + 1;
        child_argv[j] = (char*)malloc(len);
        strncpy(child_argv[j], argv[i], len);
    }

    // nochdir, noclose
    daemon(1, 1);

    int cnt = 10;
    time_t start_tt = 0;
    while (cnt > 0) {
        sleep(1);

        time_t cur_tt = time(NULL);
        // service time < 10s, exit
        if (cur_tt - start_tt < 10) {
            puts("service time too short!\n");
            pexit(-30);
        }
        // service time < 1h, --cnt
        if (cur_tt - start_tt < 3600) {
            --cnt;
        }
        start_tt = cur_tt;
        
        pid_t pid = fork();
        if (pid < 0) {
            perrno_exit();
        }

        // parent process
        if (pid > 0) {
            int status = 0;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) &&
                WEXITSTATUS(status) == EXEC_FAIL_EXITCODE) {
                pexit(-20);
            }
        }

        // child process
        if (pid == 0) {
            int ret = execvp(argv[1], child_argv);
            if (ret < 0) {
                printf("exec %s failed!\n", argv[1]);
                pexit(EXEC_FAIL_EXITCODE);
            }
        }
    }

    return 0;
}

