#include "common.h"

#define TIME_BUFSIZE 25
static char time_strbuf[TIME_BUFSIZE] = {0};

// WARNING: This function is neither reentrant nor thread-safe.
char* print_time() {
    time_t timer;
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(time_strbuf, TIME_BUFSIZE, "%Y-%m-%d %H:%M:%S", tm_info);
    return time_strbuf;
}

char tohexchar(int n) {
    if (n < 10)
        return '0' + n;
    else
        return 'a' + n - 10;
}

void print_buf(uchar* buf, int len)
{
    char *str = malloc(len * 3);
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
            logfile = fp;
            debug("logfile successfully reopened");
        } else {
            fatal("cannot open new logfile");
        }
    }
}

void init_sigactions(void)
{
    struct sigaction sighup, sigusr1;
    sighup.sa_handler = sighup_action;
    sigemptyset(&sighup.sa_mask);
    sighup.sa_flags = 0;
    sigaction(SIGHUP, NULL, &sighup);

    sigusr1.sa_handler = sigusr1_action;
    sigemptyset(&sigusr1.sa_mask);
    sighup.sa_flags = 0;
    sigaction(SIGUSR1, NULL, &sigusr1);
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

