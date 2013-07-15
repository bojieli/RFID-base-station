#include "common.h"
#include "sms.h"

#define MAX_HEADERS_LENGTH 300
void sms_send(char* msg) {
    char *encode_msg = urlencode(msg);
    char *mobile_number = get_config("sms.mobiles");
    char *encode_mobile = urlencode(mobile_number);
    int alloc_size = strlen(encode_msg) + strlen(encode_mobile) + MAX_HEADERS_LENGTH;
    char *body = malloc(alloc_size);
    snprintf(body, alloc_size, "token=%s&msg=%s&mobile=%s", get_config("sms.token"), encode_msg, encode_mobile);
    char *recvbuf = NULL;
    http_post(
        get_config("sms.remote_host"),
        atoi(get_config("sms.remote_port")),
        get_config("sms.remote_path"),
        body, strlen(body), &recvbuf);
    free(body);
    free(encode_mobile);
    free(encode_msg);
}
