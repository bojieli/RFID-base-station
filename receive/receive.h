extern pthread_mutex_t lock_sender;
unsigned char* send_queue;
unsigned int send_queue_len;
unsigned int send_queue_size;

void add_to_queue(unsigned char* buf, size_t len);
void cron_send(void);
