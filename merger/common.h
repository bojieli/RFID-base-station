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

#define IF_ERROR(expr,msg) if ((expr) == -1) { fprintf(stderr, "[%d] " msg "\n", errno); return 1; }
#define debug printf

#define bool unsigned char
#define true 1
#define false 0

#include "config.h"

// notify queue: timeout enqueue, sender dequeue
extern pthread_mutex_t lock_notify_queue;
// timers: server and timeout read and write
extern pthread_mutex_t lock_timers;

// server.c
int init_server(void);

// timeout.c
extern char *notify_queue;
extern int notify_queue_len;
extern int notify_queue_alloc_size;

void init_timeout(void);
void clear_timeout(char* key);
void set_timeout(char* key);

// sender.c
int init_sender(void);

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
int recvn(int fd, void* buf, size_t size);
int sendn(int fd, void* buf, size_t size);
int http_send(char* buf, size_t len, char** recvbuf);

#endif
