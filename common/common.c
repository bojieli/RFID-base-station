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
