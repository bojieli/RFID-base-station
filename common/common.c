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
