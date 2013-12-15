#include "common.h"
#include "receive.h"
#include "nrf.h"
#include <sys/time.h>
#include "sms.h"
#include "sys/wait.h"

static void on_irq(void);
#include "helper.c"

static bool nrf_is_working = false;

// received data from nRF24l01
static void on_irq(void)
{
    if (0 != pthread_mutex_trylock(&irq_lock))
        return;

    // first byte is channel number
    uchar buf[1 + BUF_SIZE] = {current_channel};
    int flag = nRF24L01_RxPacket(buf + 1);

    pthread_mutex_unlock(&irq_lock);

    if (flag) {
        nrf_is_working = true;
        add_to_queue(buf, 1 + BUF_SIZE);
        blink_led();
        if (LOG_VERBOSE())
            print_buf(buf, 1 + BUF_SIZE);
    } else {
        fatal("Receive failed on IRQ\n");
    }
}

static void cron_check_nrf_working()
{
    while (true) {
        int interval = atoi(get_config("nrf.check_working_interval"));
        if (interval < 1) {
            fatal("nrf.check_working_interval must be a positive integer");
            exit(1);
        }
        sleep(interval);
        if (!nrf_is_working) {
            fatal("nrf did not receive anything for %d seconds, exiting", interval);
            return;
        }
        nrf_is_working = false;
    }
}

int main(int argc, char** argv)
{
    init_params(argc, argv);
    common_init();

    pthread_mutex_init(&lock_sender, NULL);
    pthread_t tid_sender, tid_logflusher, tid_channel_switcher;
    pthread_create(&tid_sender, NULL, (void * (*)(void *))&cron_send, NULL);
    pthread_create(&tid_logflusher, NULL, (void * (*)(void *))&cron_logflush, NULL);
    pthread_create(&tid_channel_switcher, NULL, (void * (*)(void *))&cron_channel_switch, NULL);

    cron_check_nrf_working();
    if (logfile) {
        fflush(logfile);
    }
    close_fds(0, -1); // close all fds except 0-2
    execvp(argv[0], argv); // restart self

    // should never reach here
    return 0;
}
