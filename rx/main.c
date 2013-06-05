#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "nrf.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static pthread_mutex_t irq_lock;
struct timeval begin;
int sockfd; // socket to master
int batch_count = 0;

static void print_buf(uchar* buf)
{
    int i;
    for (i=0; i<BUF_SIZE; i++)
        printf("%02x ", buf[i]);
}

static void batch(uchar* buf)
{
    static int total = 0, wrong = 0;
    static int last = 0;
    static int last_time = 0, led_last_time = 0;
    static bool led = false;
    ++total;
    if (buf[BUF_SIZE-1] != 0 && buf[BUF_SIZE-1] != last + 1) {
        ++wrong;
    }
    last = buf[BUF_SIZE-1];

    if (total % batch_count == 0) {
        struct timeval now;
        gettimeofday(&now, NULL);
        int total_time = (now.tv_usec - begin.tv_usec) / 1000 + (now.tv_sec - begin.tv_sec) * 1000;
        if (total_time - led_last_time > 300) {
            led = led ? false : true;
            digitalWrite(LED_PIN, led);
            led_last_time = total_time;
        }
        print_buf(buf);
        printf("time %6d diff %6d rate %7d ", total_time, total_time - last_time, (int)(total*1000000.0/total_time));
        last_time = total_time;
        printf("total %8d wrong %8d rate %lf", total, wrong, wrong * 1.0 / total);
        printf("\n");
        fflush(stdout);
    }
}

// received data from nRF24l01
static void on_irq(void)
{
    uchar buf[BUF_SIZE];

    if (0 != pthread_mutex_trylock(&irq_lock))
        return;

    if (nRF24L01_RxPacket(buf)) {
        uchar sockbuf[BUF_SIZE+1] = {0xFF}; // 0xFF is header
        memcpy(sockbuf+1, buf, BUF_SIZE);
        if (-1 == send(sockfd, sockbuf, BUF_SIZE+1, 0)) {
            printf("socket error\n");
        }

        if (batch_count > 0)
            batch(buf);
        else {
            print_buf(buf);
            printf("\n");
        }
    } else {
        printf("Receive failed on IRQ\n");
    }

    pthread_mutex_unlock(&irq_lock);
}


static int init_send()
{
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("error creating socket\n");
        return 1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_aton("127.0.0.1", &server_addr.sin_addr);
    bzero(&(server_addr.sin_zero), 8);
    if (-1 == connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))) {
        printf("error connecting master\n");
        return 1;
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (getuid() != 0) {
        printf("You must be superuser!\n");
        return 1;
    }
    if (init_send())
        return 1;

    pthread_mutex_init(&irq_lock, NULL);

    if (wiringPiSetup() == -1) {
        printf("Cannot setup GPIO ports\n");
        return 1;
    }

    pinMode(CE_PIN, OUTPUT);
    pinMode(CSN_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);

    pinMode(IRQ_PIN, INPUT);
    pullUpDnControl(IRQ_PIN, PUD_UP);
    wiringPiISR(IRQ_PIN, INT_EDGE_FALLING, on_irq);

    gettimeofday(&begin, NULL);
    
    // init SPI with channel 0, speed 8M
    if (wiringPiSPISetup(0, 8000000) == -1) {
        printf("Cannot initialize SPI\n");
        return 1;
    }

    int station = 0;
    if (argc >= 2)
        station = atoi(argv[1]);
    init_NRF24L01(station & 0x7F); // maximum 127 channels

    if (argc == 3)
        batch_count = atoi(argv[2]);
    else
        batch_count = 0;

    printf("Initialized channel %d in %s mode.\n", station, batch_count ? "batch" : "single");
    pthread_mutex_lock(&irq_lock);
    print_configs();
    pthread_mutex_unlock(&irq_lock);

    // infinite wait
    while (1) {
        sleep(1000000);
    }
    return 0;
}
