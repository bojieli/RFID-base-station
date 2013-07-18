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

// "format" arg is separated, in case it is not constant 
#define debug(format, ...) { \
    fprintf(logfile, "[%d] %s:%d\t", (int)time(NULL), __FILE__, __LINE__); \
    fprintf(logfile, (format), ##__VA_ARGS__); \
    fprintf(logfile, "\n"); \
}

#define fatal(format, ...) debug("FATAL: " format, ##__VA_ARGS__)

#define IF_ERROR(expr,msg) if ((expr) == -1) { debug("assertion failed: %s (errno %d)", msg, errno); return 1; }

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
char* get_config(char* key);

// global configs
extern int PACKET_SIZE;
extern int ID_SIZE;
extern int REQUEST_SIZE;

#endif
