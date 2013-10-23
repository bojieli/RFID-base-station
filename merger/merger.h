#ifndef __MERGER_H__
#define __MERGER_H__

#include "common.h"

#define HEAD_SAMPLE_LEN 5 // should be odd to break tie
#define TAIL_SAMPLE_LEN 5
#define HEAD_COUNTER_SIZE 4 // assert: (1<<HEAD_COUNT_SIZE) > HEAD_SAMPLE_LEN
#define TAIL_COUNTER_SIZE 4

// student state
typedef struct {
    unsigned int head_master_count : HEAD_COUNTER_SIZE;
    unsigned int head_slave_count : HEAD_COUNTER_SIZE;
    unsigned int tail : TAIL_SAMPLE_LEN;
    unsigned int tail_count : TAIL_COUNTER_SIZE;
} student_state;

typedef union {
    student_state s;
    int i;
} state2int;

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
extern bool receiver_alive[];

#endif
