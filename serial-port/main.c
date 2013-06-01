#include <stdio.h>
#include <wiringSerial.h>
#include <sys/time.h>

#define BUF_LEN 8

struct timeval begin;

void print_packet(unsigned char buf[BUF_LEN])
{
    static int total = 0, wrong = -1;
    static unsigned char last = 0;
    int flag;
    if (last + 1 == buf[BUF_LEN-1] || (last == 0xFE && buf[BUF_LEN-1] == 0))
        flag = 0;
    else {
        ++wrong;
        flag = 1;
    }
    last = buf[BUF_LEN-1];
    ++total;
    if (flag || total % 1000 == 0) {
        struct timeval now;
        gettimeofday(&now, NULL);
        int total_time = (now.tv_usec - begin.tv_usec) / 1000 + (now.tv_sec - begin.tv_sec) * 1000;
        int i;
        for (i=0; i<BUF_LEN; i++)
            printf("%02x ", buf[i]);
        printf("time %6d rate %7d ", total_time, (int)(total*1000000.0/total_time));
        printf("total %d\t wrong %d\t rate %lf\t", total, wrong, wrong * 1.0 / total);
        printf("\n");
        fflush(stdout);
    }
}

int main(int argc, char** argv)
{
    int fd = serialOpen("/dev/ttyAMA0", 115200);
    if (fd == -1) {
        printf("Failed opening serial port\n");
        return 1;
    }
    printf("Initialized %d\n", fd);
    gettimeofday(&begin, NULL);

    int count = 0;
    unsigned char buf[BUF_LEN];
    while (1) {
        unsigned char ch = serialGetchar(fd);
        if (ch == 0xFF)
            count = 0;
        else {
            buf[count++] = ch;
            if (count == BUF_LEN)
                print_packet(buf);
        }
    }
}
