#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <sys/inotify.h>

#define true 1
#define false 0

#define MASTER 0
#define SLAVE 1

void update_background(int line, bool onoff);
void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string);

extern struct timeval last_recv_time[2];
extern pthread_mutex_t recv_time_lock;

void init_updater(void);
void init_timer(void);

#endif // __COMMON_H__
