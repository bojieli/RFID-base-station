#include "common.h"

pthread_mutex_t lock_notify_queue;
pthread_mutex_t lock_timers;

dict students, timers;

int PACKET_SIZE;
int ID_SIZE;
int REQUEST_SIZE;

static void load_global_configs() {
    PACKET_SIZE = atoi(get_config("student.packet_size"));
    if (PACKET_SIZE < 1) {
        fatal("Invalid config: student.packet_size");
        exit(1);
    }
    ID_SIZE = PACKET_SIZE * 2;
/* request format: (in ASCII)
 * | Product ID in Hex  | Action | separator |
 * | 0101xxxxxxxxxxxxxx |   0/1  |      .    |
 * Note that these chars must be in urlencode, i.e. contain only
 * alphanumeric chars, "-", "_" and ".".
 */
    REQUEST_SIZE = ID_SIZE + 2;
}

int main() {
    if (!load_config()) {
        fatal("config file does not exist");
        return 1;
    }
    load_global_configs();

    pthread_mutex_init(&lock_notify_queue, NULL);
    pthread_mutex_init(&lock_timers, NULL);

    students = new_dict();
    timers = new_dict();

    pthread_t tid_server, tid_timeout, tid_sender, tid_watchdog;
    pthread_create(&tid_server,   NULL, (void * (*)(void *))&init_server,   NULL);
    pthread_create(&tid_timeout,  NULL, (void * (*)(void *))&init_timeout,  NULL);
    pthread_create(&tid_sender,   NULL, (void * (*)(void *))&init_sender,   NULL);
    pthread_create(&tid_watchdog, NULL, (void * (*)(void *))&init_watchdog, NULL);

    pthread_join(tid_server, NULL);
    debug("Program terminated");
    return 0;
}
