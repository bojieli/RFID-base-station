#include "common.h"
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "nrf.h"
#include <sys/time.h>
#include "sms.h"

static void on_irq(void);
#include "helper.c"

int batch_count = 0;
bool test_multiple_mode = false;
bool sms_active = false;

static void batch(uchar* buf)
{
    static int total = 0, wrong = 0;
    static int last = 0;
    static int last_time = 0;
    ++total;
    if (buf[BUF_SIZE-1] != 0 && buf[BUF_SIZE-1] != last + 1) {
        ++wrong;
    }
    last = buf[BUF_SIZE-1];

    if (total % batch_count == 0) {
        struct timeval now;
        gettimeofday(&now, NULL);
        int total_time = (now.tv_usec - begin.tv_usec) / 1000 + (now.tv_sec - begin.tv_sec) * 1000;
        print_buf(buf, BUF_SIZE);
        printf("time %6d diff %6d rate %7d ", total_time, total_time - last_time, (int)(total*1000000.0/total_time));
        last_time = total_time;
        printf("total %8d wrong %8d rate %lf", total, wrong, wrong * 1.0 / total);
        printf("\n");
        fflush(stdout);
    }
}

static void test_multiple(uchar* buf)
{
    static int total[256] = {0};
    static int counter[256] = {0};
    static bool died[256] = {0};
    static int sms_count[256] = {0};
    static int last_time = 0;
    struct timeval now;
    gettimeofday(&now, NULL);
    int curr_time = (now.tv_usec - begin.tv_usec) / 1000 + (now.tv_sec - begin.tv_sec) * 1000;
    int i;
    for (i=0; i<BUF_SIZE-1; i++)
        if (buf[i] != i+1) {
            printf("invalid packet: ");
            print_buf(buf, BUF_SIZE);
            printf("\n");
            fflush(stdout);
            return;
        }
    uchar no = buf[BUF_SIZE-1];
    ++total[no];
    ++counter[no];
    if (curr_time - last_time > atoi(get_config("test.multiple_send_interval"))) {
        bool error_flag = false;
        for (i=0; i<256; i++) {
            if (total[i]) {
                printf("%d:%d ", i, counter[i]);
                if (counter[i] == 0) {
                    printf("[number %d miss]  ", i);
                    if (sms_active && !died[i] && sms_count[i]++ < atoi(get_config("sms.maxnum_per_rfid"))) {
                        char buf[100];
                        sprintf(buf, "RFID Number %d miss [5005]\n", i);
                        sms_send(buf);
                    }
                    died[i] = true;
                    error_flag = true;
                } else {
                    if (sms_active && died[i] && sms_count[i]++ < atoi(get_config("sms.maxnum_per_rfid"))) {
                        char buf[100];
                        sprintf(buf, "RFID Number %d is OK [5005]\n", i);
                        sms_send(buf);
                    }
                    died[i] = false;
                }
            }
            counter[i] = 0;
        }
        printf("\n");
        fflush(stdout);
        digitalWrite(LED2_PIN, error_flag);
        last_time = curr_time;
    }
}

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
        if (test_multiple_mode)
            test_multiple(buf);
        else if (batch_count > 0)
            batch(buf);
        else {
            print_buf(buf, BUF_SIZE);
            printf("\n");
        }
    } else {
        printf("Receive failed on IRQ\n");
    }

}

#define CONFIG_FILE "../config/receive.ini"
int main(int argc, char** argv)
{
    logfile = stderr; 
    if (!load_config(CONFIG_FILE)) {
        fatal("error parsing config file");
        exit(1);
    }
    common_init();

    if (argc == 1) {
        printf("WARNING: This program is for testing only!\n");
        printf("Usage: ./test-receiver <channel> [-m] [-n]\n");
        printf("   -m: test multiple mode\n");
        printf("   -n: mobile message notify\n");
    }

    int station = atoi(get_config("nrf.channel"));
    if (argc >= 2)
        station = atoi(argv[1]);
    init_NRF24L01(station & 0x7F); // maximum 127 channels

    if (argc >= 3) {
        if (strcmp(argv[2], "-m") == 0)
            test_multiple_mode = true;
        else
            batch_count = atoi(argv[2]);
    }
    else
        batch_count = 0;

    if (argc >= 4 && strcmp(argv[3], "-n") == 0) {
        sms_active = true;
    }

    char* msg = malloc(100);
    sprintf(msg, "Initialized channel %d in %s mode%s.\n", station,
        test_multiple_mode ? "test multiple" : (batch_count ? "batch" : "single"),
        sms_active ? " with sms notify" : "");
    printf("%s", msg);
    //sms_send(msg);
    free(msg);
    
    pthread_mutex_lock(&irq_lock);
    print_configs();
    pthread_mutex_unlock(&irq_lock);

    // infinite wait
    while (1) {
        sleep(1000000);
    }
    return 0;
}
