#define LISTEN_HOST "0.0.0.0"
#define LISTEN_PORT 12345

#define PACKET_SIZE 9
#define STUDENT_TIMEOUT 10 // in seconds

#define ACCESS_TOKEN "helloworld123"
#define REMOTE_HOST "api.shi6.com"
#define REMOTE_PATH "/ecard/notify"
#define HTTP_METHOD "POST"
#define UA_VERSION "1.0"

#define REQUEST_TIMEOUT 3000
#define REQUEST_INTERVAL 1 // in seconds
#define STUDENT_NO_SIZE PACKET_SIZE
#define REQUEST_SIZE (STUDENT_NO_SIZE+1) // student no followed by one byte of action

