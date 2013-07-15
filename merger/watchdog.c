#include "common.h"

#define MSG_MAXLEN 1024
bool report_it_now(char* format, ...) {
    va_list arg;
    va_start(arg, format);
    char msg[MSG_MAXLEN] = {0};
    vsnprintf(msg, MSG_MAXLEN, format, arg);
    va_end(arg);

    debug("REPORT IT NOW: %s", msg);
    char* recv_buf = NULL;
    return http_send(get_config("paths.reportitnow"), msg, strlen(msg), &recv_buf);
}

static int get_lock_timeout() {
    int conf = atoi(get_config("watchdog.lock_timeout"));
    if (conf < 1) {
        fatal("config watchdog.lock_timeout should be a positive integer");
        conf = 1;
    }
    return conf;
}

static void check_lock(pthread_mutex_t *lock, const char* msg) {
    struct timespec timeout;
    timeout.tv_sec = get_lock_timeout();
    timeout.tv_nsec = 0;

    if (0 == pthread_mutex_timedlock(lock, &timeout)) { // lock acquired
        pthread_mutex_unlock(lock);
        return;
    }
    report_it_now("watchdog: cannot acquire lock '%s' in %ld seconds", msg, timeout.tv_sec);
}

void init_watchdog() {
    while (1) {
        check_lock(&lock_notify_queue, "notify queue");
        check_lock(&lock_timers, "timers");
        sleep(get_lock_timeout());
    }
}
