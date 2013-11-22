#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MERGER_HOST "192.168.0.1"
#define MERGER_PORT 12345

int sockfd;

static int init_send()
{
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (sockfd == -1) {
        printf("error creating socket\n");
        return 1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MERGER_PORT);
    inet_aton(MERGER_HOST, &server_addr.sin_addr);
    bzero(&(server_addr.sin_zero), 8);
    if (-1 == connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))) {
        printf("error connecting master\n");
        return 1;
    }
    return 0;
}

#define PACK_LEN 9
int main()
{
    if (init_send())
        return 1;
    unsigned char buf[PACK_LEN] = {0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    while (1) {
        int i;
        unsigned char checksum = 0;
        for (i=0; i<PACK_LEN-1; i++)
            checksum ^= buf[i];
        buf[PACK_LEN-1] = checksum;

        for (i=0; i<PACK_LEN; i++)
            printf("%02x ", buf[i]);
        printf("\n");
        if (PACK_LEN != send(sockfd, buf, PACK_LEN, 0)) {
            printf("send error\n");
            return 1;
        }
        sleep(1);
    }
}
