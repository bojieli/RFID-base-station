#include "merger.h"

static void send_heartbeat() {
    char* recv_buf = NULL;
    debug("heartbeat");
    cloud_send(get_config("paths.heartbeat"), "", &recv_buf);
}

static bool check_heartbeat() {
    static int last_time = 0;
    int now = (int)time(NULL);
    bool should_send = (now - last_time > atoi(get_config("cloud.heartbeat_interval")));
    if (should_send)
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
    if (notify_queue_len > get_config("http.max_data_len")) {
        report_it_now("Notify queue too long (%d bytes)", notify_queue_len);
        send_len = get_config("http.max_data_len");
    } else
        send_len = 0; // get full queue length after acquiring the lock

    pthread_mutex_lock(&lock_notify_queue);

    if (send_len == 0) // not exceeding max_data_len
        send_len = notify_queue_len;

    send_buf = safe_malloc(send_len+1);
    // send from tail of queue (should be called stack?)
    memcpy(send_buf, notify_queue + notify_queue_len - send_len, send_len);
    send_buf[send_len] = '\0';

    notify_queue_len -= send_len;

    pthread_mutex_unlock(&lock_notify_queue);

send: {
    debug("sending notify queue (length %d)...", send_len);
    char* recv_buf = NULL;
    int received_bytes = cloud_send(get_config("paths.upload"), send_buf, &recv_buf);
    if (received_bytes > 0) {
        if (strncmp(recv_buf, get_config("cloud.ok_response"), received_bytes) == 0) {
            free(send_buf);
            send_buf = NULL;
        } else {
            recv_buf = realloc(recv_buf, received_bytes+1);
            recv_buf[received_bytes] = '\0';
            debug("cloud returned: %s (%s expected)", recv_buf, get_config("cloud.ok_response"));
        }
        free(recv_buf);
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

