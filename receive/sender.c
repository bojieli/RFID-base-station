#include "common.h"
#include "receive.h"

pthread_mutex_t lock_sender;

static unsigned char* send_queue;
static unsigned int send_queue_len;
static unsigned int send_queue_size;
static int sockfd = -1;

void add_to_queue(unsigned char* buf, size_t len) {
    pthread_mutex_lock(&lock_sender);

    if (send_queue_len + len > send_queue_size) {
        send_queue_size = (send_queue_size + len) * 2;
        send_queue = realloc(send_queue, send_queue_size);
        debug("realloc send_queue size to %d", send_queue_size);
    }
    memcpy(send_queue + send_queue_len, buf, len);
    send_queue_len += len;

    pthread_mutex_unlock(&lock_sender);
}

static bool try_connect(void)
{
    struct sockaddr_in server_addr;
    IF_ERROR(sockfd = socket(AF_INET, SOCK_STREAM, 0), "create socket")

    unsigned int timeout_ms = atoi(get_config("master.send_timeout"));
    struct timeval timeout = {.tv_sec = timeout_ms / 1000, .tv_usec = (timeout_ms % 1000) * 1000};
    IF_ERROR(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)), "set recv timeout")
    IF_ERROR(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)), "set send timeout")

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(get_config("master.port")));
    inet_aton(get_config("master.ip"), &server_addr.sin_addr);
    bzero(&(server_addr.sin_zero), 8);
    if (-1 == connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))) {
        fatal("error connecting master");
        close(sockfd);
        sockfd = -1; // mark it as inactive
        return false;
    }
    return true;
}

static bool do_send(void) {
    static bool lastok = true;
    static unsigned char *sendbuf;
    static unsigned int sendlen;

    if (lastok) {
        pthread_mutex_lock(&lock_sender);
        sendbuf = malloc(send_queue_len);
        memcpy(sendbuf, send_queue, send_queue_len);
        sendlen = send_queue_len;
        send_queue_len = 0;
        pthread_mutex_unlock(&lock_sender);
    }

    int sendedlen = send(sockfd, sendbuf, sendlen, 0);
    if (sendedlen < sendlen) { // send not success
        fatal("error sending, queue length %d", sendlen);
        lastok = false;
        close(sockfd);
        sockfd = -1;
        return false;
    }
    else { // send ok
        lastok = true;
        free(sendbuf);
        return true;
    }
}

static void delay_conf(const char* conf) {
    usleep(1000 * atoi(get_config(conf)));
}

/* state machine:
 *
 * begin => connect => send
 *
 * connect:
 *  - fail => delay "master.retry_interval" => connect
 *  - ok   => send
 * send (socket timeout: "master.send_timeout")
 *  - fail => connect
 *  - ok   => delay "master.send_interval" => send
 */

void cron_send(void) {
connect:
    if (!try_connect()) {
        delay_conf("master.retry_interval");
        goto connect;
    }
send:
    if (do_send()) {
        delay_conf("master.send_interval");
        goto send;
    } else {
        goto connect;
    }
}
