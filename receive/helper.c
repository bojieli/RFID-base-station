// this file should not be compiled separately

int IRQ_PIN, CSN_PIN, CE_PIN, LED_PIN, LED2_PIN;
struct timeval begin;
static pthread_mutex_t irq_lock;

static void blink_led()
{
    static int led_last_time;
    static bool led = false;
    struct timeval now;
    gettimeofday(&now, NULL);
    int total_time = (now.tv_usec - begin.tv_usec) / 1000 + (now.tv_sec - begin.tv_sec) * 1000;
    if (total_time - led_last_time > atoi(get_config("led.blink_interval"))) {
        digitalWrite(LED_PIN, (led = ~led));
        led_last_time = total_time;
    }
}

static void load_hex_config(const char* name, int* len, uchar** buf) {
    char* str = get_config(name);
    if (str[0] != '0' || (str[1] != 'x' && str[1] != 'X')) {
        goto error;
    } else {
        str += 2;
        int origlen = strlen(str);
        *len = origlen/2;
        if (*len*2 != origlen) // odd number of hex
            goto error;
        *buf = (uchar*)malloc(*len);
        str += 2;
        uchar *cur = *buf;
        while (*str != '\0') {
            *cur = hex2int(*str++) << 4;
            *cur += hex2int(*str++);
        }
    }
    return;
error:
    *len = 0;
    *buf = NULL;
}

static void load_global_configs() {
    IRQ_PIN = atoi(get_config("pin.IRQ"));
    CSN_PIN = atoi(get_config("pin.CSN"));
    CE_PIN  = atoi(get_config("pin.CE"));
    LED_PIN = atoi(get_config("pin.LED"));
    LED2_PIN = atoi(get_config("pin.LED2"));

    load_hex_config("nrf.TX_ADDRESS", &TX_ADR_WIDTH, &TX_ADDRESS);
    load_hex_config("nrf.RX_ADDRESS", &RX_ADR_WIDTH, &RX_ADDRESS);
}

static void common_init(void)
{
    load_global_configs();

    if (getuid() != 0) {
        fatal("You must be superuser!");
        exit(1);
    }

    pthread_mutex_init(&irq_lock, NULL);

    if (wiringPiSetup() == -1) {
        fatal("Cannot setup GPIO ports");
        exit(1);
    }

    pinMode(CE_PIN, OUTPUT);
    pinMode(CSN_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, 0);
    pinMode(LED2_PIN, OUTPUT);
    digitalWrite(LED2_PIN, 0);

    pinMode(IRQ_PIN, INPUT);
    pullUpDnControl(IRQ_PIN, PUD_UP);
    wiringPiISR(IRQ_PIN, INT_EDGE_FALLING, on_irq);

    gettimeofday(&begin, NULL);
    
    if (wiringPiSPISetup(atoi(get_config("spi.channel")), atoi(get_config("spi.speed"))) == -1) {
        fatal("Cannot initialize SPI");
        exit(1);
    }
}

