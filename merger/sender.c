#include "common.h"

static void send_heartbeat() {
    char* recv_buf = NULL;
    debug("heartbeat");
    http_send("", 0, &recv_buf);
}

static bool check_heartbeat() {
    static int last_time = 0;
    int now = (int)time(NULL);
    bool should_send = (now - last_time > atoi(get_config("cloud.heartbeat_interval")));
    last_time = now;
    return should_send;
}

static void commit_notify_queue() {
    static char *send_buf = NULL;
    static int send_len = 0;

    if (send_buf) {
        debug("last send command failed, resend it");
        goto send;
    }

    if (notify_queue_len == 0)
        return;

    pthread_mutex_lock(&lock_notify_queue);

    send_buf = malloc(notify_queue_len);
    memcpy(send_buf, notify_queue, notify_queue_len);
    send_len = notify_queue_len;
    notify_queue_len = 0;

    pthread_mutex_unlock(&lock_notify_queue);

send: {
    debug("sending notify queue (length %d)...", send_len);
    char* recv_buf = NULL;
    if (http_send(send_buf, send_len, &recv_buf) > 0 && strcmp(recv_buf, get_config("cloud.ok_response")) == 0) {
        free(send_buf);
        send_buf = NULL;
    }
}
}

int init_sender() {
    debug("sender thread begin");
    while (1) {
        commit_notify_queue();
        sleep(atoi(get_config("cloud.request_interval"))); // sleep to prevent from exhausting the server
        if (check_heartbeat())
            send_heartbeat();
    }
}

