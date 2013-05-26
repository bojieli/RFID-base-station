#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "nrf.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

static pthread_mutex_t irq_lock;
struct timeval begin;

// received data from nRF24l01
void on_irq(void)
{
    static int total = 0, wrong = -1;
    static int last = 0;
    uchar buf[BUF_SIZE];

    if (0 != pthread_mutex_trylock(&irq_lock))
        return;

    if (nRF24L01_RxPacket(buf)) {
        ++total;
        int this = (buf[2] << 8) + buf[3];
        if (buf[3] != 0 && this != last + 1) {
            struct timeval now;
            gettimeofday(&now, NULL);
            int total_time = (now.tv_usec - begin.tv_usec) / 1000 + (now.tv_sec - begin.tv_sec) * 1000;
            int i;
            for (i=0; i<BUF_SIZE; i++)
                printf("%02x ", buf[i]);
            ++wrong;
            printf("time %6d rate %7d ", total_time, (int)(total*1000000.0/total_time));
            printf("total %d\t wrong %d\t rate %lf\t", total, wrong, wrong * 1.0 / total);
            printf("\n");
        }
        last = this;
    } else {
        printf("Receive failed on IRQ\n");
    }

    pthread_mutex_unlock(&irq_lock);
}

int main(int argc, char** argv)
{
    if (getuid() != 0) {
        printf("You must be superuser!\n");
        return 1;
    }

    pthread_mutex_init(&irq_lock, NULL);

    if (wiringPiSetup() == -1) {
        printf("Cannot setup GPIO ports\n");
        return 1;
    }

    pinMode(CE_PIN, OUTPUT);
    pinMode(CSN_PIN, OUTPUT);

    pinMode(IRQ_PIN, INPUT);
    pullUpDnControl(IRQ_PIN, PUD_UP);
    wiringPiISR(IRQ_PIN, INT_EDGE_FALLING, on_irq);

    gettimeofday(&begin, NULL);
    
    // init SPI with channel 0, speed 8M
    if (wiringPiSPISetup(0, 1000000) == -1) {
        printf("Cannot initialize SPI\n");
        return 1;
    }

    int station = 0;
    if (argc == 2)
        station = atoi(argv[1]);
    init_NRF24L01(station & 0x7F); // maximum 127 channels

    printf("Initialized channel %d.\n", station);
    pthread_mutex_lock(&irq_lock);
    print_configs();
    pthread_mutex_unlock(&irq_lock);

    // infinite wait
    while (1) {
        sleep(1000000);
    }
    return 0;
}
