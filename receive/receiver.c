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
        print_buf(buf, BUF_SIZE);
    } else {
        fatal("Receive failed on IRQ\n");
    }
}

int forked_main(int argc, char** argv)
{
    init_params(argc, argv);
    common_init();

    pthread_mutex_init(&lock_sender, NULL);
    pthread_t tid_sender;
    pthread_create(&tid_sender, NULL, (void * (*)(void *))&cron_send, NULL);

    int station = atoi(get_config("nrf.channel"));
    init_NRF24L01(station & 0x7F); // maximum 127 channels

    debug("Initialized channel %d\n", station);
    
    pthread_mutex_lock(&irq_lock);
    print_configs();
    pthread_mutex_unlock(&irq_lock);

    // if sender finds error, exit
    pthread_join(tid_sender, NULL);
    return 0;
}

int main(int argc, char** argv)
{
    while (1) {
        int flag = fork();
        if (flag == -1) {
            fatal_stderr("fork failed");
            return 1;
        }
        if (flag == 0) { // child
            forked_main(argc, argv);
            return 0;
        } else {
            debug_stderr("child %d created", flag);
            waitpid(flag, NULL, 0);
            debug_stderr("child %d terminated", flag);
        }
        sleep(1); // prevent fast respawn
    }
}
