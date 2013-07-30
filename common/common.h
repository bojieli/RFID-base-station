#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <ctype.h>
#include <stdarg.h>

extern FILE *logfile;

// common.c
char* print_time(void);
char tohexchar(int n);

// strip path in __FILE__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// This macro is not thread-safe.
#define generic_debug(outfd, format, ...) { \
    fprintf(outfd, "[%s] %s:%d\t", print_time(), __FILENAME__, __LINE__); \
    fprintf(outfd, (format), ##__VA_ARGS__); \
    fprintf(outfd, "\n"); \
    fflush(outfd); \
}
#define debug(...) generic_debug(logfile, __VA_ARGS__)
#define debug_stderr(...) generic_debug(stderr, __VA_ARGS__)

#define fatal(format, ...) debug("FATAL: " format, ##__VA_ARGS__)
#define fatal_stderr(format, ...) debug_stderr("FATAL: " format, ##__VA_ARGS__)

// fail fast to find bugs earlier
#define __ASSERT(expr,msg) if (!(expr)) { \
    fatal("assertion failed: %s (errno %d)", (msg), errno); \
    exit(1); \
}
#define IF_ERROR(expr,msg) __ASSERT(((expr) != -1), msg)
#define ASSERT(expr) __ASSERT((expr), #expr)

#define bool unsigned char
#define true 1
#define false 0

// dict.c
typedef struct dict_item {
    char* key;
    int value;
    struct dict_item* next;
} dict_item;
typedef dict_item* dict;

extern dict students, timers;

dict new_dict(void);
int get(dict d, char* key);
bool set(dict d, char* key, int value);
void __remove(dict_item* curr);
bool del(dict d, char* key);

// http.c
#define MAX_HEADERS_LENGTH 300
int recvn(int fd, void* buf, size_t size);
int sendn(int fd, void* buf, size_t size);
int http_post(const char* host, int port, const char* path, char* body, size_t len, char** recvbuf);
char* urlencode(char* msg);

// config.c
bool load_config(const char* config_file);
char* get_config(const char* key);

// global configs
extern int PACKET_SIZE;
extern int ID_SIZE;
extern int REQUEST_SIZE;

#endif
