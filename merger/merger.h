#ifndef __MERGER_H__
#define __MERGER_H__

#include "common.h"

#define CONFIG_FILE "merger.ini"

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

// watchdog.c
void init_watchdog(void);
bool report_it_now(char* format, ...);

#endif
