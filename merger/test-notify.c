#include "merger.h"

char *notify_queue = NULL;
int notify_queue_len = 0;
int notify_queue_alloc_size = 0;
pthread_mutex_t lock_notify_queue;

int PACKET_SIZE, ID_SIZE, REQUEST_SIZE;

static void notify(char* student_no, bool action) {
    pthread_mutex_lock(&lock_notify_queue);

    if (notify_queue_len + REQUEST_SIZE > notify_queue_alloc_size) {
        notify_queue_alloc_size = (notify_queue_alloc_size + REQUEST_SIZE) * 2;
        notify_queue = safe_realloc(notify_queue, notify_queue_alloc_size);
    }
    memcpy(notify_queue + notify_queue_len, student_no, ID_SIZE);
    notify_queue[notify_queue_len + REQUEST_SIZE-2] = '0' + action;
    notify_queue[notify_queue_len + REQUEST_SIZE-1] = '.';
    notify_queue_len += REQUEST_SIZE;

    pthread_mutex_unlock(&lock_notify_queue);
}

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

static void gen_student_id(char* str) {
    unsigned char id[PACKET_SIZE];
    id[0] = id[1] = 0x01;
    int i;
    for (i=2; i<PACKET_SIZE-1; i++)
        id[i] = rand() & 0xFF;
    id[PACKET_SIZE-1] = 0;
    for (i=0; i<PACKET_SIZE-1; i++)
        id[PACKET_SIZE-1] ^= id[i];
    for (i=0; i<PACKET_SIZE; i++) {
        str[i<<1] = tohexchar(id[i] >> 4);
        str[(i<<1)+1] = tohexchar(id[i] & 0xF);
    }
}

int main(int argc, char **argv) {
    int speed = 10; // 10 events per second
    if (argc == 2)
        speed = atoi(argv[1]);
    int interval = 1000000 / speed;

    logfile = stderr;
    if (!load_config("/opt/gewuit/rfid/etc/merger.ini")) {
        fatal("error parsing config file");
        exit(1);
    }
    load_global_configs();
    pthread_mutex_init(&lock_notify_queue, NULL);
    srand(time(NULL));

    pthread_t tid_server;
    pthread_create(&tid_server, NULL, (void*(*)(void*))&init_sender, NULL);

    while (true) {
        char student_id[ID_SIZE];
        gen_student_id(student_id);
        notify(student_id, true);
        usleep(interval);
        notify(student_id, false);
        usleep(interval);
    }
}
