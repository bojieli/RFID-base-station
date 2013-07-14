#include "common.h"

static char tohexchar(int n) {
    if (n < 10)
        return '0' + n;
    else
        return 'a' + n - 10;
}

static void commit_notify_queue() {
    static char *send_buf = NULL;
    static int send_len = 0;

    if (send_buf) {
        debug("last send command failed, resend it\n");
        goto send;
    }

    if (notify_queue_len == 0)
        return;

    pthread_mutex_lock(&lock_notify_queue);

    send_buf = malloc(notify_queue_len * 2);
    int i;
    for (i=0; i<notify_queue_len; i++) {
        send_buf[i*2] = tohexchar(notify_queue[i] >> 4);
        send_buf[i*2+1] = tohexchar(notify_queue[i] & 0xF);
    }
    send_len = notify_queue_len * 2;
    notify_queue_len = 0;

    pthread_mutex_unlock(&lock_notify_queue);

send: {
    debug("sending notify queue (length %d)...\n", send_len);
    char* recv_buf = NULL;
    if (http_send(send_buf, send_len, &recv_buf) > 0 && strcmp(recv_buf, get_config("cloud.ok_response")) == 0) {
        free(send_buf);
        send_buf = NULL;
    }
}
}

int init_sender() {
    debug("sender thread begin\n");
    while (1) {
        commit_notify_queue();
        sleep(atoi(get_config("cloud.request_interval"))); // sleep to prevent from exhausting the server
    }
}


