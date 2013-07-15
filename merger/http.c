#include "common.h"

// receive until error or filled up the buffer
int recvn(int fd, void* buf, size_t size) {
    int recvtotal = 0;
    while (recvtotal < size) {
        int recvlen = recv(fd, buf, size - recvtotal, 0);
        if (recvlen == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
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
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
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

// note: buf should only include urlencoded chars
// return received bytes on success, -1 on failure
int http_send(char* buf, size_t len, char** recvbuf) {
    struct sockaddr_in server_addr;
    int sockfd;
    IF_ERROR((sockfd = socket(AF_INET, SOCK_STREAM, 0)), "socket")
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(get_config("cloud.remote_port")));
    struct hostent *he = gethostbyname(get_config("cloud.remote_host"));
    IF_ERROR((he == NULL ? -1 : 0), "gethostbyname")
    memcpy(&server_addr.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
    bzero(&(server_addr.sin_zero), 8);
    IF_ERROR(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)), "connect")
    debug("connected to server at %s:%s", get_config("cloud.remote_port"), get_config("cloud.remote_host"));

#define MAX_HEADERS_LENGTH 300
    char *body = malloc(len + MAX_HEADERS_LENGTH);
    snprintf(body, len + MAX_HEADERS_LENGTH,
        "token=%s&data=%s",
        get_config("cloud.access_token"),
        buf);
    char *tcp = malloc(len + MAX_HEADERS_LENGTH);
    snprintf(tcp, len + MAX_HEADERS_LENGTH,
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n%s",
        get_config("cloud.http_method"),
        get_config("cloud.remote_path"),
        get_config("cloud.remote_host"),
        get_config("cloud.user_agent"),
        (int)strlen(body), body);
    free(body);

    IF_ERROR(sendn(sockfd, tcp, strlen(tcp)), "send to remote server")
    free(tcp);

#define BUF_SIZE 1024
    int totalbytes = 0;
    char *received = malloc(BUF_SIZE);
    bool in_payload = false, isok = false;
    while (1) {
        int recvbytes = recv(sockfd, received, BUF_SIZE, 0);
        if (recvbytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                continue;
            break;
        }
        if (recvbytes == 0) // end of connection
            break;

        if (in_payload) {
            *recvbuf = realloc(*recvbuf, totalbytes + recvbytes);
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
                    *recvbuf = malloc(newbytes);
                    totalbytes = newbytes;
                    in_payload = true;
                }
            }
        }
    }
    if (isok) {
        debug("HTTP connection OK, %d bytes received", totalbytes);
    } else {
        debug("HTTP connection failed, server returned:\n%s", *recvbuf);
    }
    return totalbytes;
}

