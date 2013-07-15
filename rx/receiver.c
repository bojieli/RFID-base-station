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
#include "sms.h"

#define BLINK_INTERVAL 100 // in ms

static pthread_mutex_t irq_lock;
struct timeval begin;
int sockfd; // socket to master

static void print_buf(uchar* buf)
{
    int i;
    for (i=0; i<BUF_SIZE; i++)
        printf("%02x ", buf[i]);
}

static void blink_led()
{
    static int led_last_time;
    static bool led = false;
    struct timeval now;
    gettimeofday(&now, NULL);
    int total_time = (now.tv_usec - begin.tv_usec) / 1000 + (now.tv_sec - begin.tv_sec) * 1000;
    if (total_time - led_last_time > BLINK_INTERVAL) {
        digitalWrite(LED_PIN, (led = ~led));
        led_last_time = total_time;
    }
}

// received data from nRF24l01
static void on_irq(void)
{
    uchar buf[BUF_SIZE];

    if (0 != pthread_mutex_trylock(&irq_lock))
        return;
    int flag = nRF24L01_RxPacket(buf);
    pthread_mutex_unlock(&irq_lock);

    if (flag) {
        if (-1 == send(sockfd, buf, BUF_SIZE, 0)) {
            printf("socket error\n");
        }

        blink_led();
        print_buf(buf);
        printf("\n");
    } else {
        printf("Receive failed on IRQ\n");
    }
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
    digitalWrite(LED_PIN, 0);
    pinMode(LED2_PIN, OUTPUT);
    digitalWrite(LED2_PIN, 0);

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

    printf("Initialized channel %d\n", station);
    
    pthread_mutex_lock(&irq_lock);
    print_configs();
    pthread_mutex_unlock(&irq_lock);

    // infinite wait
    while (1) {
        sleep(1000000);
    }
    return 0;
}
