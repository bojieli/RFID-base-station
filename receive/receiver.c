#include "common.h"
#include "receive.h"
#include "nrf.h"
#include <sys/time.h>
#include "sms.h"
#include "sys/wait.h"

static void on_irq(void);
#include "helper.c"

// received data from nRF24l01
static void on_irq(void)
{
    if (0 != pthread_mutex_trylock(&irq_lock))
        return;

    uchar buf[BUF_SIZE];
    int flag = nRF24L01_RxPacket(buf);

    pthread_mutex_unlock(&irq_lock);

    if (flag) {
        add_to_queue(buf, BUF_SIZE);
        blink_led();
        if (LOG_VERBOSE())
            print_buf(buf, BUF_SIZE);
    } else {
        fatal("Receive failed on IRQ\n");
    }
}

int main(int argc, char** argv)
{
    init_params(argc, argv);
    common_init();

    pthread_mutex_init(&lock_sender, NULL);
    pthread_t tid_sender, tid_logflusher;
    pthread_create(&tid_sender, NULL, (void * (*)(void *))&cron_send, NULL);
    pthread_create(&tid_logflusher, NULL, (void * (*)(void *))&cron_logflush, NULL);

    int station = atoi(get_config("nrf.channel"));
    init_NRF24L01(station & 0x7F); // maximum 127 channels

    debug("Initialized channel %d\n", station);
    
    pthread_mutex_lock(&irq_lock);
    print_configs();
    pthread_mutex_unlock(&irq_lock);

    // if sender finds error, exit
    pthread_join(tid_sender, NULL);
    fatal("sender thread exit");
    return 0;
}
