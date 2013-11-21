#include "common.h"

/* Notes on Linux recv and send in blocking mode:
 * They may return -1 with errno EINTR in case of interrupts,
 *   so these cases must be handled as if no error occured.
 * They may return partial count or zero with errno EAGAIN
 *   or EWOULDBLOCK in case of timeout, so the request fails.
 */

// receive until error or filled up the buffer
int recvn(int fd, void* buf, size_t size) {
    int recvtotal = 0;
    while (recvtotal < size) {
        int recvlen = recv(fd, buf, size - recvtotal, 0);
        if (recvlen == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (recvlen == 0)
            return recvtotal;
        recvtotal += recvlen;
        buf += recvlen;
    }
    return recvtotal;
}

// send until error or all sent
int sendn(int fd, void* buf, size_t size) {
    int sendtotal = 0;
    while (sendtotal < size) {
        int sendlen = send(fd, buf, size - sendtotal, 0);
        if (sendlen == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (sendlen == 0)
            return size;
        sendtotal += sendlen;
        buf += sendlen;
    }
    return sendtotal;
}

// note: body should only include urlencoded chars
// return received bytes on success, -1 on failure
int http_post(const char* remote_host, int remote_port, const char* remote_path, char* body, size_t len, char** recvbuf) {
    ASSERT(recvbuf != NULL)

#define MY_IF_ERROR(exp, msg) if (-1 == (exp)) { debug("assertion failed: %s (errno %d)", (msg), errno); goto out; }

    // dynamically allocated
    char *tcp = NULL;
    // the return value
    int totalbytes = 0;
    // HTTP request succeed?
    bool isok = false;

    unsigned int start_time = (unsigned int)time(NULL);

    struct sockaddr_in server_addr;
    int sockfd;
    MY_IF_ERROR((sockfd = socket(AF_INET, SOCK_STREAM, 0)), "socket")
    struct timeval timeout = {.tv_sec = atoi(get_config("http.timeout")), .tv_usec = 0};
    MY_IF_ERROR(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)), "set recv timeout")
    MY_IF_ERROR(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)), "set send timeout")
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(remote_port);
    struct hostent *he = gethostbyname(remote_host);
    MY_IF_ERROR((he == NULL ? -1 : 0), "gethostbyname")
    memcpy(&server_addr.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
    bzero(&(server_addr.sin_zero), 8);
    MY_IF_ERROR(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)), "connect")
    debug("connected to server at %s:%d", inet_ntoa(server_addr.sin_addr), remote_port);

    tcp = safe_malloc(len + MAX_HEADERS_LENGTH);
    snprintf(tcp, len + MAX_HEADERS_LENGTH,
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n%s",
        remote_path,
        remote_host,
        get_config("http.user_agent"),
        (unsigned int)len, body);

    int sendlen = strlen(tcp);
    int sendedlen = send(sockfd, tcp, sendlen, 0);
    if (sendedlen < sendlen) {
        fatal("send to remote server: %d bytes sent, total %d bytes", sendedlen, sendlen);
        goto out;
    }

#define BUF_SIZE 4096
    char buf[BUF_SIZE] = {0};
    bool in_payload = false;
    while (1) {
        unsigned int curr_time = (unsigned int)time(NULL);
        if (curr_time - start_time > timeout.tv_sec) {
            fatal("HTTP request timeout, start %u, now %u", start_time, curr_time);
            goto out;
        }
        int recvbytes = recv(sockfd, buf, BUF_SIZE, 0);
        if (recvbytes == -1) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (recvbytes == 0) // end of connection
            break;

        if (in_payload) {
            *recvbuf = safe_realloc(*recvbuf, totalbytes + recvbytes);
            memcpy(*recvbuf + totalbytes, buf, recvbytes);
            totalbytes += recvbytes;
        } else {
            if (!isok) {
                // assume that these special bytes are not separated by packet border
                char* okpos = strstr(buf, "200 OK\r\n");
                if (okpos != NULL)
                    isok = true;
            }
            if (isok) { // this <if> should not be replaced by <else>
                char* payload = strstr(buf, "\r\n\r\n");
                if (payload != NULL) {
                    payload += 4;
                    int newbytes = recvbytes - (payload - buf);
                    *recvbuf = safe_malloc(newbytes);
                    memcpy(*recvbuf, payload, newbytes);
                    totalbytes = newbytes;
                    in_payload = true;
                }
            }
        }
    }
out:
    if (tcp != NULL)
        free(tcp);
    close(sockfd);
    debug("HTTP connection %s, %d bytes received, remote path %s", (isok ? "OK" : "failed"), totalbytes, remote_path);
    return totalbytes;
}

char* urlencode(char* msg) {
    char* buf = safe_malloc(strlen(msg) * 3 + 1);
    char* cur = buf;
    const char hexchars[17] = "0123456789abcdef";
    while (*msg != '\0') {
        if ((*msg>='a' && *msg<='z') || (*msg>='A' && *msg<='Z') || (*msg>='0' && *msg<='9') || *msg=='.' || *msg=='-' || *msg=='*' || *msg=='_')
            *cur++ = *msg;
        //else if (*msg==' ')
        //    *cur++ = '+';
        else {
            *cur++ = '%';
            *cur++ = hexchars[*msg >> 4];
            *cur++ = hexchars[*msg & 0xF];
        }
        msg++;
    }
    *cur = '\0';
    return buf;
}

