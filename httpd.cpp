#include "h.h"

#define DEFAULT_WORKER_PROCESSES    4
#define MAXNUM_WORKER_PROCESSES     1024
static proc_ctx_t s_worker_processes[MAXNUM_WORKER_PROCESSES];

typedef struct conf_ctx_s {
    IniParser* parser;
    int loglevel;
    int worker_processes;
    int port;
} conf_ctx_t;
conf_ctx_t g_conf_ctx;

inline void conf_ctx_init(conf_ctx_t* ctx) {
    ctx->parser = new IniParser;
    ctx->loglevel = LOG_LEVEL_DEBUG;
    ctx->worker_processes = 0;
    ctx->port = 0;
}

static void print_version();
static void print_help();

static int  parse_confile(const char* confile);

static int  signal_init();
static void signal_cleanup();
static void handle_signal();

static void master_proc(void* userdata);
static void worker_proc(void* userdata);

// short options
static const char options[] = "hvc:ts:dp:";
// long options
static const option_t long_options[] = {
    {'h', "help",       NO_ARGUMENT},
    {'v', "version",    NO_ARGUMENT},
    {'c', "confile",    REQUIRED_ARGUMENT},
    {'t', "test",       NO_ARGUMENT},
    {'s', "signal",     REQUIRED_ARGUMENT},
    {'d', "daemon",     NO_ARGUMENT},
    {'p', "port",       REQUIRED_ARGUMENT}
};
static const char detail_options[] = R"(
  -h|--help                 Print this information
  -v|--version              Print version
  -c|--confile <confile>    Set configure file, default etc/{program}.conf
  -t|--test                 Test Configure file and exit
  -s|--signal <signal>      Send <signal> to process,
                            <signal>=[start,stop,restart,status]
  -d|--daemon               Daemonize
  -p|--port <port>          Set listen port
)";

void print_version() {
    printf("%s version %s\n", g_main_ctx.program_name, get_compile_version());
}

void print_help() {
    printf("Usage: %s [%s]\n", g_main_ctx.program_name, options);
    printf("Options:\n%s\n", detail_options);
}

int parse_confile(const char* confile) {
    conf_ctx_init(&g_conf_ctx);
    int ret = g_conf_ctx.parser->LoadFromFile(confile);
    if (ret != 0) {
        printf("Load confile [%s] failed: %d\n", confile, ret);
        exit(-40);
    }

    // loglevel
    const char* szLoglevel = g_conf_ctx.parser->GetValue("loglevel").c_str();
    if (stricmp(szLoglevel, "DEBUG") == 0) {
        g_conf_ctx.loglevel = LOG_LEVEL_DEBUG;
    } else if (stricmp(szLoglevel, "INFO") == 0) {
        g_conf_ctx.loglevel = LOG_LEVEL_INFO;
    } else if (stricmp(szLoglevel, "WARN") == 0) {
        g_conf_ctx.loglevel = LOG_LEVEL_WARN;
    } else if (stricmp(szLoglevel, "ERROR") == 0) {
        g_conf_ctx.loglevel = LOG_LEVEL_ERROR;
    } else {
        g_conf_ctx.loglevel = LOG_LEVEL_DEBUG;
    }
    hlog_set_level(g_conf_ctx.loglevel);

    // worker_processes
    int worker_processes = 0;
    worker_processes = atoi(g_conf_ctx.parser->GetValue("worker_processes").c_str());
    if (worker_processes <= 0 || worker_processes > MAXNUM_WORKER_PROCESSES) {
        worker_processes = get_ncpu();
        hlogd("worker_processes=ncpu=%d", worker_processes);
    }
    if (worker_processes <= 0 || worker_processes > MAXNUM_WORKER_PROCESSES) {
        worker_processes = DEFAULT_WORKER_PROCESSES;
    }
    g_conf_ctx.worker_processes = worker_processes;

    // port
    int port = 0;
    const char* szPort = get_arg("p");
    if (szPort) {
        port = atoi(szPort);
    }
    if (port == 0) {
        port = atoi(g_conf_ctx.parser->GetValue("port").c_str());
    }
    if (port == 0) {
        printf("Please config listen port!\n");
        exit(-10);
    }
    g_conf_ctx.port = port;

    return 0;
}

#ifdef OS_UNIX
// unix use signal
// we use SIGTERM to quit process
#define SIGNAL_TERMINATE    SIGTERM
#include <sys/wait.h>

void signal_handler(int signo) {
    hlogi("pid=%d recv signo=%d", getpid(), signo);
    switch (signo) {
    case SIGINT:
    case SIGNAL_TERMINATE:
        hlogi("killall processes");
        signal(SIGCHLD, SIG_IGN);
        for (int i = 0; i < MAXNUM_WORKER_PROCESSES; ++i) {
            if (s_worker_processes[i].pid <= 0) break;
            kill(s_worker_processes[i].pid, SIGKILL);
            s_worker_processes[i].pid = -1;
        }
        exit(0);
        break;
    case SIGCHLD:
    {
        pid_t pid = 0;
        int status = 0;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            hlogw("proc stop/waiting, pid=%d status=%d", pid, status);
            for (int i = 0; i < MAXNUM_WORKER_PROCESSES; ++i) {
                if (s_worker_processes[i].pid == pid) {
                    s_worker_processes[i].pid = -1;
                    create_proc(&s_worker_processes[i]);
                    break;
                }
            }
        }
    }
        break;
    default:
        break;
    }
}

int signal_init() {
    signal(SIGINT, signal_handler);
    signal(SIGCHLD, signal_handler);
    signal(SIGNAL_TERMINATE, signal_handler);

    atexit(signal_cleanup);
    return 0;
}

void signal_cleanup() {
}
#elif defined(OS_WIN)
// win32 use Event
static HANDLE s_hEventTerm = NULL;

#include <mmsystem.h>
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
void WINAPI on_timer(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
    DWORD ret = WaitForSingleObject(s_hEventTerm, 0);
    if (ret == WAIT_OBJECT_0) {
        timeKillEvent(uTimerID);
        hlogi("pid=%d recv event [TERM]", getpid());
        exit(0);
    }
}

int signal_init() {
    char eventname[MAX_PATH] = {0};
    snprintf(eventname, sizeof(eventname), "%s_term_event", g_main_ctx.program_name);
    s_hEventTerm = CreateEvent(NULL, FALSE, FALSE, eventname);
    //s_hEventTerm = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventname);

    timeSetEvent(1000, 1000, on_timer, 0, TIME_PERIODIC);

    atexit(signal_cleanup);
    return 0;
}

void signal_cleanup() {
    CloseHandle(s_hEventTerm);
    s_hEventTerm = NULL;
}
#endif

void handle_signal() {
    const char* signal = get_arg("s");
    if (signal) {
        if (strcmp(signal, "start") == 0) {
            if (g_main_ctx.oldpid > 0) {
                printf("%s is already running, pid=%d\n", g_main_ctx.program_name, g_main_ctx.oldpid);
                exit(0);
            }
        } else if (strcmp(signal, "stop") == 0) {
            if (g_main_ctx.oldpid > 0) {
#ifdef OS_UNIX
                kill(g_main_ctx.oldpid, SIGNAL_TERMINATE);
#else
                SetEvent(s_hEventTerm);
#endif
                printf("%s stop/waiting\n", g_main_ctx.program_name);
            } else {
                printf("%s is already stopped\n", g_main_ctx.program_name);
            }
            exit(0);
        } else if (strcmp(signal, "restart") == 0) {
            if (g_main_ctx.oldpid > 0) {
#ifdef OS_UNIX
                kill(g_main_ctx.oldpid, SIGNAL_TERMINATE);
#else
                SetEvent(s_hEventTerm);
#endif
                printf("%s stop/waiting\n", g_main_ctx.program_name);
                msleep(1000);
            }
        } else if (strcmp(signal, "status") == 0) {
            if (g_main_ctx.oldpid > 0) {
                printf("%s start/running, pid=%d\n", g_main_ctx.program_name, g_main_ctx.oldpid);
            } else {
                printf("%s stop/waiting\n", g_main_ctx.program_name);
            }
            exit(0);
        } else {
            printf("Invalid signal: '%s'\n", signal);
            exit(0);
        }
        printf("%s start/running\n", g_main_ctx.program_name);
    }
}

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int main(int argc, char** argv) {
    // g_main_ctx
    main_ctx_init(argc, argv);
    if (argc == 1) {
        print_help();
        exit(10);
    }
    //int ret = parse_opt(argc, argv, options);
    int ret = parse_opt_long(argc, argv, long_options, ARRAY_SIZE(long_options));
    if (ret != 0) {
        print_help();
        exit(ret);
    }

    /*
    printf("---------------arg------------------------------\n");
    printf("%s\n", g_main_ctx.cmdline);
    for (auto& pair : g_main_ctx.arg_kv) {
        printf("%s=%s\n", pair.first.c_str(), pair.second.c_str());
    }
    for (auto& item : g_main_ctx.arg_list) {
        printf("%s\n", item.c_str());
    }
    printf("================================================\n");
    */

    /*
    printf("---------------env------------------------------\n");
    for (auto& pair : g_main_ctx.env_kv) {
        printf("%s=%s\n", pair.first.c_str(), pair.second.c_str());
    }
    printf("================================================\n");
    */

    // help
    if (get_arg("h")) {
        print_help();
        exit(0);
    }

    // version
    if (get_arg("v")) {
        print_version();
        exit(0);
    }

    // logfile
    hlog_set_file(g_main_ctx.logfile);
    hlogi("%s version: %s", g_main_ctx.program_name, get_compile_version());

    // confile
    const char* confile = get_arg("c");
    if (confile) {
        strncpy(g_main_ctx.confile, confile, sizeof(g_main_ctx.confile));
    }

    // g_conf_ctx
    parse_confile(g_main_ctx.confile);

    // test
    if (get_arg("t")) {
        printf("Test confile [%s] OK!\n", g_main_ctx.confile);
        exit(0);
    }

    // signal
    signal_init();
    handle_signal();

#ifdef OS_UNIX
    // daemon
    if (get_arg("d")) {
        // nochdir, noclose
        int ret = daemon(1, 1);
        if (ret != 0) {
            printf("daemon error: %d\n", ret);
            exit(-10);
        }
        // parent process exit after daemon, so pid changed.
        g_main_ctx.pid = getpid();
    }
    // proctitle
    char proctitle[256] = {0};
    snprintf(proctitle, sizeof(proctitle), "%s: master process", g_main_ctx.program_name);
    setproctitle(proctitle);
#endif

    // pidfile
    create_pidfile();
    hlogi("%s start/running, pid=%d", g_main_ctx.program_name, g_main_ctx.pid);

    // socket => bind => listen
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(errno);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(g_conf_ctx.port);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(errno);
    }
    if (listen(sock, 64) < 0) {
        perror("listen");
        exit(errno);
    }

    // master-worker proc
    memset(s_worker_processes, 0, sizeof(s_worker_processes));
    for (int i = 0; i < g_conf_ctx.worker_processes; ++i) {
        proc_ctx_t* ctx = &s_worker_processes[i];
        snprintf(ctx->proctitle, sizeof(ctx->proctitle), "%s: worker process", g_main_ctx.program_name);
        ctx->proc = worker_proc;
        ctx->userdata = (void*)(long)(sock);
        create_proc(ctx);
    }
    master_proc(NULL);

    return 0;
}

void master_proc(void* userdata) {
    while(1) msleep(1000);
}

#include "http_status.h"
#define DOCUMENT_ROOT   "html"
#define HOME_PAGE       "index.html"
typedef struct http_request_s {
    string method;
    string uri;
    string version;
    keyval_t headers;
    string body;
} http_request_t;

typedef struct http_response_s {
    int status_code;
    string status_str;
    keyval_t headers;
    string body;
} http_response_t;

int parse_http_request(const string& str, http_request_t* req) {
    const char* buf = str.c_str();
    // request line
    const char* p = buf;
    const char* delim = strstr(p, "\r\n");
    if (delim == NULL)  return -10;
    // method
    const char* space = strchr(p, ' ');
    req->method = std::string(p, space-p);
    p = space + 1;
    // uri
    space = strchr(p, ' ');
    req->uri = std::string(p, space-p);
    p = space + 1;
    // version
    req->version = std::string(p, delim-p);
    p = delim + 2;
    // headers
    while (1) {
        delim = strstr(p, "\r\n");
        if (delim == NULL)  break;
        if (delim == p) {
            p = delim + 2;
            break; // headers end
        }
        const char* colon = strchr(p, ':');
        if (colon) {
            // k: v
            req->headers[std::string(p, colon-p)] = std::string(colon+2, delim-(colon+2));
        }
        p = delim + 2;
    }
    // body
    req->body = p;

    printf("------------------http_request------------------------\n");
    printf("%s %s %s\r\n", req->method.c_str(), req->uri.c_str(), req->version.c_str());
    for(auto& header : req->headers) {
        printf("%s: %s\r\n", header.first.c_str(), header.second.c_str());
    }
    printf("\r\n");
    printf("%s\n", req->body.c_str());
    printf("------------------http_request------------------------\n");
    return 0;
}

int build_http_response(http_response_t& res, string* str) {
    res.headers["Server"] = "httpd/1.19.3.15";
    datetime_t now = get_datetime();
    res.headers["Date"] = asprintf("%04d-%02d-%02d %02d:%02d:%02d",
            now.year,
            now.month,
            now.day,
            now.hour,
            now.min,
            now.sec);
    res.headers["Content-Type"] = "text/html";
    res.headers["Content-Length"] = std::to_string(res.body.size());
    res.headers["Connection"] = "close";

    str->clear();
    // status line
    str->append(asprintf("%s %d %s\r\n", "HTTP/1.1", res.status_code, res.status_str.c_str()));
    // headers
    for(auto& header : res.headers) {
        str->append(asprintf("%s: %s\r\n", header.first.c_str(), header.second.c_str()));
    }
    str->append("\r\n");
    // body
    str->append(res.body);
    return 0;
}

void worker_proc(void* userdata) {
    // accpet => recv => parse_http_request => build_http_response => send => close
    int sock = (int)(long)(userdata);
    struct sockaddr_in addr;
    char buf[1500] = {0};
    while (1) {
        // accept
        memset(&addr, 0, sizeof(addr));
        socklen_t addrlen = sizeof(addr);
        int connsock = accept(sock, (struct sockaddr*)&addr, &addrlen);
        if (connsock < 0) {
            perror("accept");
            exit(errno);
        }
        printf("accept [%s:%d]\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        ScopeCleanup _(close, connsock);

        // recv
        ssize_t recvbytes = recv(connsock, buf, sizeof(buf), 0);
        if (recvbytes < 0) {
            perror("recv");
            continue;
        }
        printf("recv:\n%s\n", buf);

        // parse_http_request
        http_request_t req;
        http_response_t res;
        int ret = parse_http_request(buf, &req);
        // build_http_response
        if (ret != 0) {
            res.status_code = HTTP_BAD_REQUEST;
            res.status_str = get_http_status_string(HTTP_BAD_REQUEST);
            make_http_status_page(res.status_code, res.status_str.c_str(), res.body);
        }

        if (strcmp(req.method.c_str(), "GET") != 0) {;
            res.status_code = 501;
            res.status_str = "Unimplemented Method";
            make_http_status_page(res.status_code, res.status_str.c_str(), res.body);
        } else {
            HFile file;
            string filepath = DOCUMENT_ROOT;
            filepath += req.uri;
            if (strcmp(req.uri.c_str(), "/") == 0) {
                filepath += HOME_PAGE;
            }
            if (file.open(filepath.c_str(), "r") != 0) {
                res.status_code = HTTP_NOT_FOUND;
                res.status_str = get_http_status_string(HTTP_NOT_FOUND);
                make_http_status_page(res.status_code, res.status_str.c_str(), res.body);
            } else {
                res.status_code = HTTP_OK;
                res.status_str = get_http_status_string(HTTP_OK);
                file.readall(res.body);
            }
        }
        string res_str;
        build_http_response(res, &res_str);

        // send
        ssize_t sendbytes = send(connsock, res_str.c_str(), res_str.size(), 0);
        if (sendbytes < 0) {
            perror("send");
            continue;
        }
        printf("send:\n%s\n", res_str.c_str());
    }
}
