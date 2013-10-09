#include "common.h"

#define TIME_BUFSIZE 30
static char time_strbuf[TIME_BUFSIZE] = {0};

// WARNING: This function is neither reentrant nor thread-safe.
char* print_time() {
    struct timeval tv;
    struct tm* tm_info;

    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);

    strftime(time_strbuf, TIME_BUFSIZE, "%Y-%m-%d %H:%M:%S", tm_info);
    int len = strlen(time_strbuf);
    sprintf(time_strbuf + len, ".%03d", (int)tv.tv_usec / 1000);
    return time_strbuf;
}

char tohexchar(int n) {
    if (n < 10)
        return '0' + n;
    else
        return 'a' + n - 10;
}

int hex2int(char c) {
    if (c>='a' && c<='f')
        return c-'a'+10;
    else if (c>='A' && c<='F')
        return c-'A'+10;
    else if (c>='0' && c<='9')
        return c-'0';
    return -1;
}

void print_buf(uchar* buf, int len)
{
    char *str = safe_malloc(len * 3);
    int i;
    for (i=0; i<len; i++) {
        str[i*3 + 0] = tohexchar(buf[i] >> 4);
        str[i*3 + 1] = tohexchar(buf[i] & 15);
        str[i*3 + 2] = ' ';
    }
    str[len*3 - 1] = '\0';

    debug("received ID: %s", str);

    free(str);
}

static void sighup_action(int signo) {
    debug("caught signal SIGHUP");
    reload_configs();
}

FILE *logfile = NULL;
char *logfile_saved = NULL;

static void sigusr1_action(int signo) {
    debug("caught signal SIGUSR1");
    if (logfile_saved) {
        FILE *fp;
        fp = fopen(logfile_saved, "a");
        if (fp) {
            debug("this logfile is going to be logrotated");
            fclose(logfile);
            logfile = fp;
            debug("logfile successfully reopened");
        } else {
            fatal("cannot open new logfile");
        }
    }
}

static void sigpipe_action(int signo) {
    fatal("caught signal SIGPIPE, errorno %d, ignoring", signo);
}

void init_sigactions(void)
{
    struct sigaction sighup, sigusr1, sigpipe;
    sighup.sa_handler = sighup_action;
    sigemptyset(&sighup.sa_mask);
    sighup.sa_flags = 0;
    sigaction(SIGHUP, &sighup, NULL);

    sigusr1.sa_handler = sigusr1_action;
    sigemptyset(&sigusr1.sa_mask);
    sigusr1.sa_flags = 0;
    sigaction(SIGUSR1, &sigusr1, NULL);

    sigpipe.sa_handler = sigpipe_action;
    sigemptyset(&sigpipe.sa_mask);
    sigpipe.sa_flags = 0;
    sigaction(SIGPIPE, &sigpipe, NULL);
}

void init_params(int argc, char** argv)
{
    if (argc != 2 && argc != 3) {
        fatal_stderr("Usage: %s <config-file> [<log-file>]", argv[0]);
        exit(1);
    }
    logfile = (argc == 3 ? fopen(argv[2], "a") : stderr);
    if (logfile == NULL) {
        fatal_stderr("Cannot open logfile");
        exit(1);
    }
    logfile_saved = (argc == 3 ? strdup(argv[2]) : NULL);

    if (!load_config(argv[1])) {
        fatal("error parsing config file");
        exit(1);
    }

    init_sigactions();
}

#define MSG_MAXLEN 1024
bool report_it_now(char* format, ...) {
    va_list arg;
    va_start(arg, format);
    char msg[MSG_MAXLEN] = {0};
    vsnprintf(msg, MSG_MAXLEN, format, arg);
    va_end(arg);

    debug("REPORT IT NOW: %s", msg);
    char* recv_buf = NULL;
    int flag = cloud_send(get_config("paths.reportitnow"), msg, &recv_buf);
    if (recv_buf)
        free(recv_buf);
    return flag;
}

// return received bytes on success, -1 on failure
int cloud_send(const char* remote_path, char* buf, char** recvbuf) {
    char *encoded = urlencode(buf);
    int len = strlen(encoded);
    char *body = safe_malloc(len + MAX_HEADERS_LENGTH);
    snprintf(body, len + MAX_HEADERS_LENGTH,
        "token=%s&data=%s",
        get_config("cloud.access_token"),
        encoded);
    free(encoded);
    int flag = http_post(get_config("cloud.remote_host"),
        atoi(get_config("cloud.remote_port")),
        remote_path,
        body, strlen(body), recvbuf);
    free(body);
    return flag;
}

// flush logfile every one second
void cron_logflush(void) {
    while (true) {
        sleep(1);
        if (logfile)
            fflush(logfile);
    }
}
