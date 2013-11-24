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
#include <net/if.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <limits.h>

#ifndef NR_OPEN
#define NR_OPEN 1024
#endif

extern FILE *logfile;
extern char *logfile_saved;

// strip path in __FILE__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// This macro is not thread-safe.
#define generic_debug(outfd, prepend_text, ...) { \
    fprintf((outfd), "[%s] %s:%d\t", print_time(), __FILENAME__, __LINE__); \
    fprintf((outfd), "%s", (prepend_text)); \
    fprintf((outfd), __VA_ARGS__); \
    fprintf((outfd), "\n"); \
}
#define debug(...) generic_debug(logfile, "", __VA_ARGS__)
#define debug_stderr(...) generic_debug(stderr, "", __VA_ARGS__)

#define fatal(...) generic_debug(logfile, "FATAL: ", __VA_ARGS__)
#define fatal_stderr(...) generic_debug(stderr, "FATAL: ", __VA_ARGS__)

#define LOG_VERBOSE() (atoi(get_config("debug.log_verbose")) == 1) 
#define debug_verbose(...) if (LOG_VERBOSE()) debug(__VA_ARGS__);

// fail fast to find bugs earlier
#define __ASSERT(expr,msg) if (!(expr)) { \
    fatal("assertion failed: %s (errno %d)", (msg), errno); \
    exit(1); \
}
#define IF_ERROR(expr,msg) __ASSERT(((expr) != -1), msg)
#define ASSERT(expr) __ASSERT((expr), #expr)

#define __safe_alloc_assert(size) \
    if (__p == NULL) { \
        report_it_now("failed to allocate %d bytes", (size)); \
        exit(1); \
    }
#define safe_realloc(orig,size) ({ \
    void *__p = realloc((orig), (size)); \
    __safe_alloc_assert(size) \
    __p; \
})
#define safe_malloc(size) ({ \
    void *__p = malloc(size); \
    __safe_alloc_assert(size) \
    __p; \
})

#define bool unsigned char
#define true 1
#define false 0
#define uchar unsigned char

// common.c
char* print_time(void);
char tohexchar(int n);
int hex2int(char c);
void print_buf(uchar* buf, int len);
void init_sigactions(void);
void init_params(int argc, char** argv);
bool report_it_now(char* format, ...);
int cloud_send(const char* remote_path, char* buf, char** recvbuf);
void cron_logflush(void);
void close_fds(bool close_std, ...);
void close_all_fds(void);

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
bool reload_configs(void);

// global configs
extern int PACKET_SIZE;
extern int ID_SIZE;
extern int REQUEST_SIZE;

#endif
