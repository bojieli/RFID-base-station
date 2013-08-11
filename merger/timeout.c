#include "merger.h"

char *notify_queue = NULL;
int notify_queue_len = 0;
int notify_queue_alloc_size = 0;

static void notify(char* student_no, bool action) {
    debug("Notify: student %s action %s", student_no, action ? "IN" : "OUT");

    pthread_mutex_lock(&lock_notify_queue);

    if (notify_queue_len + REQUEST_SIZE > notify_queue_alloc_size) {
        notify_queue_alloc_size = (notify_queue_alloc_size + REQUEST_SIZE) * 2;
        notify_queue = safe_realloc(notify_queue, notify_queue_alloc_size);
        debug("realloc notify_queue size to %d", notify_queue_alloc_size);
    }
    // see merger.c for cloud request format
    memcpy(notify_queue + notify_queue_len, student_no, ID_SIZE);
    notify_queue[notify_queue_len + REQUEST_SIZE-2] = '0' + action;
    notify_queue[notify_queue_len + REQUEST_SIZE-1] = '.';
    notify_queue_len += REQUEST_SIZE;

    pthread_mutex_unlock(&lock_notify_queue);
}

void clear_timeout(char* key) {
    pthread_mutex_lock(&lock_timers);
    del(timers, key);
    pthread_mutex_unlock(&lock_timers);
}

void set_timeout(char* key) {
    pthread_mutex_lock(&lock_timers);
    set(timers, key, (int)time(NULL));
    pthread_mutex_unlock(&lock_timers);
}

static void check_timers() {
    int curr_time = (int)time(NULL);

    pthread_mutex_lock(&lock_timers);
    // WARNING: mutex is not reentrant, so do NOT lock it again before unlock!

    dict prev_timer = timers;
    while (prev_timer->next != NULL) {
        dict timer = prev_timer->next;
        if (curr_time - timer->value >= atoi(get_config("student.timeout"))) {
            int curr_state = get(students, timer->key);
            debug("student %s timeout, back to state 0", timer->key);
            set(students, timer->key, 0); // goto state 0
            if (curr_state == 3 || curr_state == 4) {
                notify(timer->key, (curr_state == 4));
            }
            __remove(prev_timer); // when student goes to state 0, timer should be removed
            continue;
        }
        prev_timer = timer;
    }

    pthread_mutex_unlock(&lock_timers);
}

void init_timeout(void) {
    debug("timeout thread begin");
    while (1) {
        check_timers();
        sleep(1); // timers are in 1 second resolution
    }
}
