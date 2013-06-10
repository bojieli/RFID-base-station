#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>
#define IF_ERROR(expr,msg) if ((expr) == -1) { fprintf(stderr, "[%d] " msg "\n", errno); return 1; }

#include "sms.h"

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
            return size;
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

int http_send(char* msg) {
    struct sockaddr_in server_addr;
    int sockfd;
    IF_ERROR((sockfd = socket(AF_INET, SOCK_STREAM, 0)), "socket")
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    struct hostent *he = gethostbyname(REMOTE_HOST);
    IF_ERROR((he == NULL ? -1 : 0), "gethostbyname")
    memcpy(&server_addr.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
    bzero(&(server_addr.sin_zero), 8);
    IF_ERROR(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)), "connect")

#define MAX_HEADERS_LENGTH 300
    int len = strlen(msg);
    char *body = malloc(len + MAX_HEADERS_LENGTH);
    snprintf(body, len + MAX_HEADERS_LENGTH,
        "token=" SMS_TOKEN "&msg=%s&mobile=" MOBILE_NUMBER,
        msg);
    char *tcp = malloc(len + MAX_HEADERS_LENGTH);
    snprintf(tcp, len + MAX_HEADERS_LENGTH,
        HTTP_METHOD " " REMOTE_PATH " HTTP/1.0\r\n"
        "Host: " REMOTE_HOST "\r\n"
        "User-Agent: rfid/" UA_VERSION "\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n%s",
        strlen(body), body);
    free(body);
    //printf("send:\n%s\n", tcp);

    IF_ERROR(sendn(sockfd, tcp, strlen(tcp)), "send to remote server")
    free(tcp);

    int recvbytes = 0;
#define BUF_SIZE 1000
    char received[BUF_SIZE+1] = {0};
    while (recvbytes == 0)
        recvbytes = recv(sockfd, received, BUF_SIZE, 0);
    //printf("received:\n%s\n", received);
    close(sockfd);
    return 0;
}

void sms_send(char* msg) {
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
    http_send(buf);
    free(buf);
}
