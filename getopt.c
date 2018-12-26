#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char version[] = "1.0.0.0";
static char options[] = "hvtdc:";
static char help[] = "\n\
Options:\n\
h               print help\n\
v               print version\n\
t               test configure\n\
d               daemon\n\
c confile       set configure file\n\
";

int     g_argc = 0;
char**  g_argv = NULL;
char*   g_configure = NULL;
char*   g_program_name = NULL;

static int show_version = 0;
static int show_help = 0;
static int show_configure = 0;
static int test_configure = 0;
static int daemon = 0;

char*   confile = NULL;

void print_help() {
    printf("Usage: %s [%s]", g_program_name, options);
    puts(help);
}

void print_version() {
    printf("%s version %s\n", g_program_name, version);
}

void print_configure() {
    printf("configure: %s\n", g_configure);
}

void save_argv(int argc, char** argv) {
    g_argc = argc;
    g_argv = (char**)malloc((argc+1)*sizeof(char*));
    if (g_argv == NULL) {
        return;
    }
    int i;
    int total_len = 0;
    for (i = 0; i < argc; ++i) {
        int len = strlen(argv[i]) + i;
        total_len += len;
        g_argv[i] = (char*)malloc(len);
        if (g_argv[i] == NULL) {
            return;
        }
        strncpy(g_argv[i], argv[i], len);
    }
    g_argv[i] = NULL;

    g_configure = (char*)malloc(total_len+1);
    if (g_configure == NULL) {
        return;
    }
    memset(g_configure, 0, total_len+1);
    for (i = 0; i < argc; ++i) {
        strcat(g_configure, argv[i]);
        strcat(g_configure, " ");
    }
}

char* filename(char* filepath) {
    char* p = filepath;
    while (*p) ++p;
    while (p >= filepath) {
        if (*p == '\\' || *p == '/') break;
        --p;
    }
    return ++p;
}

int get_options(int argc, char** argv) {
    char* p = NULL;
    g_program_name = filename(argv[0]);
    for (int i = 1; i < argc; ++i) {
        p = (char*)argv[i];
        if (*p != '-') {
            printf("invalid option: %s\n", argv[i]);
            return -1;
        }
        ++p;
        while(*p) {
            switch (*p) {
            case '?':
            case 'h':
                show_help = 1;
                show_version = 1;
                break;
            case 'v':
                show_version = 1;
                break;
            case 't':
                show_configure = 1;
                test_configure = 1;
                break;
            case 'd':
                daemon = 1;
                break;
            case 'c':
                {
                    if (*(p+1)) {
                        confile = ++p;
                    } else if (argv[i+1]) {
                        confile = argv[++i];
                    } else {
                        printf("option -%c requires parameter\n", *p);
                        return -1;
                    }
                }
                goto endwhile;
            case 's':
            default:
                printf("invalid option: %s\n", argv[i]);
                return -1;
            }
            ++p;
        }
endwhile:
        continue;
    }
    return 0;
}

int main(int argc, char** argv) {
    save_argv(argc, argv);
    if (get_options(argc, argv) != 0) {
        return -1;
    }

    if (show_configure) {
        print_configure();
    }

    if (show_version) {
        print_version();
    }

    if (show_help) {
        print_help();
    }

    return 0;
}

