#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "nrf.h"
#include <unistd.h>
#include <sys/types.h>

// received data from nRF24l01
void on_irq(void)
{
    static int total = 0;
    uchar buf[BUF_SIZE];
    int i;
    if (nRF24L01_RxPacket(buf)) {
        printf("%d\t", total++);
        for (i=0; i<BUF_SIZE; i++)
            printf("%02x ", buf[i]);
        printf("\n");
    } else {
        printf("Receive failed on IRQ\n");
    }
}

int main(int argc, char** argv)
{
    if (getuid() != 0) {
        printf("You must be superuser!\n");
        return 1;
    }
    if (wiringPiSetup() == -1) {
        printf("Cannot setup GPIO ports\n");
        return 1;
    }

    pinMode(CE_PIN, OUTPUT);
    pinMode(CSN_PIN, OUTPUT);

    pinMode(IRQ_PIN, INPUT);
    pullUpDnControl(IRQ_PIN, PUD_UP);
    
    // init SPI with channel 0, speed 1M
    if (wiringPiSPISetup(0, 8000000) == -1) {
        printf("Cannot initialize SPI\n");
        return 1;
    }

    // init with station 10
    init_NRF24L01(0);

    // set up IRQ
    wiringPiISR(IRQ_PIN, INT_EDGE_FALLING, on_irq);

    printf("Initialized.\n");

    //int i;
    //for (i=0;i<10;i++) {
    //uchar tx_buf[10];
    //nRF24L01_TxPacket(tx_buf);
    //usleep(10000);
    //}
//    while (1) {
//        on_irq();
//        usleep(10000);
//    }

    // infinite wait
    while (1) {
        sleep(1000000);
    }
    return 0;
}
