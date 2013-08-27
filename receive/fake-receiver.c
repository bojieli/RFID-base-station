#include "common.h"
#include "receive.h"

int main(int argc, char** argv)
{
    init_params(argc-1, argv+1);

    int per_second = 0;
    if (argc >= 2)
        per_second = atoi(argv[2]);
    if (per_second <= 0)
        per_second = 1;

    pthread_mutex_init(&lock_sender, NULL);
    pthread_t tid_sender;
    pthread_create(&tid_sender, NULL, (void * (*)(void *))&cron_send, NULL);

    int BUF_SIZE = atoi(get_config("nrf.RX_PLOAD_WIDTH"));
    while (true) {
        unsigned char buf[BUF_SIZE];
        buf[0] = 1;
        int i;
        for (i=1; i<BUF_SIZE-1; i++)
            buf[i] = i-1;
        buf[BUF_SIZE-1] = 0;
        for (i=0; i<BUF_SIZE-1; i++)
            buf[BUF_SIZE-1] ^= buf[i];
        add_to_queue(buf, BUF_SIZE);

        usleep(1000000 / per_second);
    }

    return 0;
}
