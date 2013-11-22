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

    uchar buf[BUF_SIZE];
    int flag = nRF24L01_RxPacket(buf);

    pthread_mutex_unlock(&irq_lock);

    if (flag) {
        nrf_is_working = true;
        add_to_queue(buf, BUF_SIZE);
        blink_led();
        if (LOG_VERBOSE())
            print_buf(buf, BUF_SIZE);
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
    pthread_t tid_sender, tid_logflusher;
    pthread_create(&tid_sender, NULL, (void * (*)(void *))&cron_send, NULL);
    pthread_create(&tid_logflusher, NULL, (void * (*)(void *))&cron_logflush, NULL);

    int station = atoi(get_config("nrf.channel"));
    init_NRF24L01(station & 0x7F); // maximum 127 channels

    debug("Initialized channel %d\n", station);
    
    pthread_mutex_lock(&irq_lock);
    print_configs();
    pthread_mutex_unlock(&irq_lock);

    cron_check_nrf_working();
    if (logfile) {
        fflush(logfile);
    }
    // all files should have been opened with O_CLOEXEC flag
    execvp(argv[0], argv); // restart self

    // should never reach here
    return 0;
}
