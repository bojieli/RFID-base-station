#include "common.h"

#define LOGFILE "/opt/gewuit/rfid/log/merger.log"

static void here(int device) {
    pthread_mutex_lock(&recv_time_lock);
    gettimeofday(&last_recv_time[device], NULL);
    pthread_mutex_unlock(&recv_time_lock);

    update_background(device, true);
}

static void parse_line(char* line, int len) {
    char *dup = strndup(line, len);
    if (strstr(dup, "master"))
        here(MASTER);
    else if (strstr(dup, "slave"))
        here(SLAVE);
    free(dup);
}

static void warn_log_deleted() {
    attron(COLOR_PAIR(4));
    print_in_middle(stdscr, 4, 0, 0, "WARNING: log file is deleted, please reopen this program");
    attroff(COLOR_PAIR(4));
}

static void nextline() {
#define MAX_LINE_LEN 1024
    static FILE *log = NULL;
    static char linebuf[MAX_LINE_LEN+1] = {0};
    static int buflen = 0;

    if (log == NULL) {
        log = fopen(LOGFILE, "re");
        if (log == NULL) {
            warn_log_deleted();
            return;
        }
        fseek(log, -1, SEEK_END);
        return;
    }

    while (true) {
        unsigned char c = fgetc(log);
        if (c == 255) // EOF
            return;
        if (c == '\n')
            break;
        if (c >= 128) // not displayable, ignore
            continue;
        linebuf[buflen++] = c;
        if (buflen == MAX_LINE_LEN) // as if there is a \n
            break;
    }
    
    // end of line
    parse_line(linebuf, buflen);
    buflen = 0;
}

void init_updater(void)
{
    nextline(); // initialize log file
    int fd = inotify_init();
    int watchfd = inotify_add_watch(fd, LOGFILE, IN_MODIFY);
    struct inotify_event e;
    while (true) {
        read(fd, &e, sizeof(e)); // block until inotify event occurs
        nextline();
    }
}

