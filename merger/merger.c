#include "common.h"

pthread_mutex_t lock_threads_running;
pthread_mutex_t lock_notify_queue;
pthread_mutex_t lock_timers;

dict students, timers;

static void* init_server_helper(void* arg) {
    init_server();
    pthread_mutex_unlock(&lock_threads_running);
    return NULL;
}
static void* init_timeout_helper(void* arg) {
    init_timeout();
    pthread_mutex_unlock(&lock_threads_running);
    return NULL;
}
static void* init_sender_helper(void* arg) {
    init_sender();
    pthread_mutex_unlock(&lock_threads_running);
    return NULL;
}

int main() {
    pthread_mutex_init(&lock_notify_queue, NULL);
    pthread_mutex_init(&lock_timers, NULL);
    pthread_mutex_init(&lock_threads_running, NULL);
    pthread_mutex_lock(&lock_threads_running);

    students = new_dict();
    timers = new_dict();

    pthread_t tid_server, tid_sender;
    pthread_create(&tid_server, NULL, &init_server_helper, NULL);
    pthread_create(&tid_server, NULL, &init_timeout_helper, NULL);
    pthread_create(&tid_sender, NULL, &init_sender_helper, NULL);

    // wait until any of the threads terminate
    // In fact, only server thread may exit
    pthread_mutex_lock(&lock_threads_running);
    debug("Program terminated\n");
    return 0;
}
