#include "common.h"
#include "nrf.h"
#include <sys/time.h>
#include "sms.h"

static void on_irq(void);
#include "helper.c"

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
            fatal("socket error\n");
        }
        blink_led();
        print_buf(buf);
        fprintf(logfile, "\n");
    } else {
        debug("Receive failed on IRQ\n");
    }
}

int main(int argc, char** argv)
{
    init_params(argc, argv);
    common_init();

    int station = atoi(get_config("nrf.channel"));
    if (argc >= 2)
        station = atoi(argv[1]);
    init_NRF24L01(station & 0x7F); // maximum 127 channels

    debug("Initialized channel %d\n", station);
    
    pthread_mutex_lock(&irq_lock);
    print_configs();
    pthread_mutex_unlock(&irq_lock);

    // infinite wait
    while (1) {
        sleep(1000000);
    }
    return 0;
}
