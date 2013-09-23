#include "merger.h"

static int get_conf(const char* conf) {
    int value = atoi(get_config(conf));
    if (value < 1) {
        fatal("config %s should be a positive integer", conf);
        value = 1;
    }
    return value;
}

static void check_lock(pthread_mutex_t *lock, const char* msg) {
    struct timespec abstime;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    abstime.tv_sec = tv.tv_sec + get_conf("watchdog.lock_timeout");
    abstime.tv_nsec = tv.tv_usec * 1000;

    int flag = pthread_mutex_timedlock(lock, &abstime);
    if (flag == 0) { // lock acquired
        pthread_mutex_unlock(lock);
        return;
    }
    report_it_now("watchdog: cannot acquire lock '%s', errno %d", msg, flag);
}

bool receiver_alive[2] = {false, false};
static bool last_receiver_alive[2] = {true, true};

/* This function should be invoked periodically.
 * receiver_alive[no] should be set to true when receive heartheat.
 */
static void check_receiver(int no, const char* name) {
    if (receiver_alive[no] == false && last_receiver_alive[no] == true) {
        report_it_now("watchdog: receiver %s dead", name);
    }
    last_receiver_alive[no] = receiver_alive[no];
    receiver_alive[no] = false;
}

void init_watchdog() {
    int relatime = 0;
    while (1) {
        sleep(1); // minimum check interval: 1 second
        check_lock(&lock_notify_queue, "notify queue");
        check_lock(&lock_timers, "timers");
        relatime++;
        if (relatime % get_conf("watchdog.receiver_timeout") == 0) {
            check_receiver(0, "master");
            check_receiver(1, "slave");
        }
    }
}
