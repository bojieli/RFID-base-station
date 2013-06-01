#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "nrf.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>

// received data from nRF24l01
void on_irq(void)
{
    printf(".");
    clear_intr();
}

int main(int argc, char** argv)
{
    if (getuid() != 0) {
        printf("You must be superuser!\n");
        return 1;
    }
    int id = 0;
    uchar buf[BUF_SIZE] = {0};
    srand((int)time(NULL));

    if (wiringPiSetup() == -1) {
        printf("Cannot setup GPIO ports\n");
        return 1;
    }

    pinMode(CE_PIN, OUTPUT);
    pinMode(CSN_PIN, OUTPUT);

    //pinMode(IRQ_PIN, INPUT);
    //pullUpDnControl(IRQ_PIN, PUD_UP);
    //wiringPiISR(IRQ_PIN, INT_EDGE_FALLING, on_irq);

    // init SPI with channel 0, speed 8M
    if (wiringPiSPISetup(0, 1000000) == -1) {
        printf("Cannot initialize SPI\n");
        return 1;
    }
    
    if (argc < 3) {
        printf("Usage: station student_id\n");
        return 1;
    }
    int station = atoi(argv[1]);
    init_NRF24L01(station & 0x7F); // maximum 127 channels
    
    id = atoi(argv[2]) & 0xFFFF;
    buf[2] = id >> 8;
    buf[3] = id & 0xFF;
    buf[4] = 0xc0;
    buf[5] = 1<<(rand() % 6);
    
    int i;
    for (i=0;i<BUF_SIZE;i++)
        printf("%02x ", buf[i]);
    printf("\n");
    
    nRF24L01_TxPacket(buf);

    return 0;
}
