#include "common.h"

pthread_mutex_t lock_notify_queue;
pthread_mutex_t lock_timers;

dict students, timers;

int main() {
    if (!load_config()) {
        printf("config file does not exist");
        return 1;
    }

    pthread_mutex_init(&lock_notify_queue, NULL);
    pthread_mutex_init(&lock_timers, NULL);

    students = new_dict();
    timers = new_dict();

    pthread_t tid_server, tid_timeout, tid_sender;
    pthread_create(&tid_server,  NULL, (void * (*)(void *))&init_server,  NULL);
    pthread_create(&tid_timeout, NULL, (void * (*)(void *))&init_timeout, NULL);
    pthread_create(&tid_sender,  NULL, (void * (*)(void *))&init_sender,  NULL);

    pthread_join(tid_server, NULL);
    debug("Program terminated\n");
    return 0;
}
