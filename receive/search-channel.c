#include "common.h"
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "nrf.h"
#include <sys/time.h>
#include "sms.h"

static void on_irq(void);
#include "helper.c"

int station = 0;

// received data from nRF24l01
static void on_irq(void)
{
    uchar buf[BUF_SIZE];

    if (0 != pthread_mutex_trylock(&irq_lock))
        return;
    int flag = nRF24L01_RxPacket(buf);
    pthread_mutex_unlock(&irq_lock);

    if (flag) {
        blink_led();
        printf("station %3d ID ", station);
        print_buf(buf, BUF_SIZE);
        printf("\n");
    } else {
        printf("Receive failed on IRQ\n");
    }
}

#define CONFIG_FILE "../config/receiver.ini"
int main(int argc, char** argv)
{
    logfile = stderr; 
    if (!load_config(CONFIG_FILE)) {
        fatal("error parsing config file");
        exit(1);
    }
    common_init();

    for (station=0; station<128; station++) {
        init_NRF24L01(station & 0x7F);
        pthread_mutex_lock(&irq_lock);
        print_configs();
        pthread_mutex_unlock(&irq_lock);
        sleep(10);
    }

    close_all_fds();
    execvp(argv[0], argv);
    return 0;
}