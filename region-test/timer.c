#include "common.h"

static int time_diff(struct timeval *t1, struct timeval *t2) {
    pthread_mutex_lock(&recv_time_lock);
    int diff = (t1->tv_sec - t2->tv_sec) * 1000
        + (t1->tv_usec - t2->tv_usec) / 1000;
    pthread_mutex_unlock(&recv_time_lock);
    return diff;
}

static void check_status() {
    struct timeval time;
    gettimeofday(&time);
    
    int dev;
    for (dev=0; dev<2; dev++) {
        if (time_diff(&time, &last_recv_time[dev]) > 1000)
            update_background(dev, false);
    }
}

void init_timer() {
    while (true) {
        check_status();
        usleep(100000);
    }
}
