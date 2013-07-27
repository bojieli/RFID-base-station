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

// note: body should only include urlencoded chars
// return received bytes on success, -1 on failure
int http_post(const char* remote_host, int remote_port, const char* remote_path, char* body, size_t len, char** recvbuf) {

    struct sockaddr_in server_addr;
    int sockfd;
    IF_ERROR((sockfd = socket(AF_INET, SOCK_STREAM, 0)), "socket")
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(remote_port);
    struct hostent *he = gethostbyname(remote_host);
    IF_ERROR((he == NULL ? -1 : 0), "gethostbyname")
    memcpy(&server_addr.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
    bzero(&(server_addr.sin_zero), 8);
    IF_ERROR(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)), "connect")
    debug("connected to server at %s:%d", inet_ntoa(server_addr.sin_addr), remote_port);

    char *tcp = malloc(len + MAX_HEADERS_LENGTH);
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

    IF_ERROR(sendn(sockfd, tcp, strlen(tcp)), "send to remote server")
    free(tcp);

#define BUF_SIZE 1024
    char buf[BUF_SIZE] = {0};
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
    close(sockfd);
    debug("HTTP connection %s, %d bytes received, remote path %s", (isok ? "OK" : "failed"), totalbytes, remote_path);
    return totalbytes;
}

char* urlencode(char* msg) {
    char* buf = malloc(strlen(msg) * 3 + 1);
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

