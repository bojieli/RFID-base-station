#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "nrf.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

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

    // init SPI with channel 0, speed 8M
    if (wiringPiSPISetup(0, 8000000) == -1) {
        printf("Cannot initialize SPI\n");
        return 1;
    }
    
    if (argc < 3) {
        printf("Usage: station student_id\n");
        return 1;
    }
    int station = atoi(argv[1]);
    init_NRF24L01(station & 0x7F); // maximum 127 channels
    print_configs();
    
    struct timeval begin;
    gettimeofday(&begin, NULL);
    int counter = 0;
    while (1) {
        id = atoi(argv[2]) & 0xFFFF;
        buf[2] = id >> 8;
        buf[3] = id & 0xFF;
        buf[4] = 0xc0;
        buf[5] = 1<<(rand() % 6);
        
        nRF24L01_TxPacket(buf);
        // buffer is invalid now

        if (++counter % 1000 == 0) {
            struct timeval now;
            gettimeofday(&now, NULL);
            int total_time = (now.tv_usec - begin.tv_usec) / 1000 + (now.tv_sec - begin.tv_sec) * 1000;
            printf("time %7d total %8d rate %7d\n", total_time, counter, (int)(counter*1000000.0/total_time));
            fflush(stdout);
        }
    }

    return 0;
}
