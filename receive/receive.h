extern pthread_mutex_t lock_sender;
extern uchar current_channel;

void add_to_queue(unsigned char* buf, size_t len);
// thread entry
void cron_send(void);
