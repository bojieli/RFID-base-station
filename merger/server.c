#include "common.h"

static char tohexchar(int n) {
    if (n < 10)
        return '0' + n;
    else
        return 'a' + n - 10;
}

static bool check_packet(unsigned char* pack) {
    unsigned char checksum = 0;
    int i;
    for (i=0; i<PACKET_SIZE; i++)
        checksum ^= pack[i];
    return (checksum == 0);
}

static void print_packet(unsigned char* pack) {
    int i;
    for (i=0; i<PACKET_SIZE; i++)
        printf("%02x ", pack[i]);
}

static void handle_student(char* id, int action) {
    const int state_table[][2] = {
        {1, 2},
        {1, 3},
        {4, 2},
        {1, 3},
        {4, 2},
    };
    int new_state = state_table[get(students, id)][action];
    set(students, id, new_state);
    clear_timeout(id);
    if (new_state != 0)
        set_timeout(id);
}

static void handle_packet(unsigned char* pack, int action) {
    if (!check_packet(pack)) {
        print_packet(pack);
        return;
    }
    char* id = malloc(ID_SIZE+1);
    int i;
    for (i=0; i<PACKET_SIZE; i++) {
        id[i*2] = tohexchar(pack[i] >> 4);
        id[i*2+1] = tohexchar(pack[i] & 0xF);
    }
    id[ID_SIZE] = '\0';
    handle_student(id, action);
    free(id);
}

static int clientfds[2] = {0};

static int msg_loop() {
    struct pollfd fds[2] = {{.fd = clientfds[0], .events = POLLIN}, {.fd = clientfds[1], .events = POLLIN}};
    while (1) {
        IF_ERROR(poll(fds, 2, -1), "poll")
        if (fds[0].revents & POLLERR || fds[1].revents & POLLERR)
            return -1;

        int i;
        for (i=0; i<2; i++) {
            if (fds[i].revents & POLLIN) {
                unsigned char* buf = malloc(PACKET_SIZE);
                int readlen = recvn(clientfds[i], buf, PACKET_SIZE);
                if (readlen == -1) {
                    fprintf(stderr, "Error: read from %s\n", i ? "slave" : "master");
                    return -1;
                }
                if (readlen == 0) {
                    debug(i ? "slave exit\n" : "master exit\n");
                    return 0;
                }
                handle_packet(buf, i);
                free(buf);
            }
        }
    }
}

int init_server()
{
    debug("server thread begin\n");

    int sockfd;
    struct sockaddr_in myaddr;
    IF_ERROR((sockfd = socket(AF_INET, SOCK_STREAM, 0)), "socket")
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(atoi(get_config("listen.port")));
    myaddr.sin_addr.s_addr = inet_addr(get_config("listen.host"));
    bzero(&(myaddr.sin_zero), 8);
    IF_ERROR(bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)), "bind")
    IF_ERROR(listen(sockfd, 10), "listen")
    debug("listening %s:%s, local ip %s\n", get_config("listen.host"), get_config("listen.port"), get_config("listen.local_ip"));

    while (1) {
        struct sockaddr_in client_addr;
        unsigned sin_size = sizeof(client_addr);
        int newfd;
        IF_ERROR((newfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)), "accept")
        char* client_addr_str = inet_ntoa(client_addr.sin_addr);
        if (0 == strcmp(client_addr_str, get_config("listen.local_ip"))) {
            clientfds[0] = newfd;
            debug("master (%s) connected\n", client_addr_str);
        } else {
            clientfds[1] = newfd;
            debug("slave (%s) connected\n", client_addr_str);
        }
        if (clientfds[0] && clientfds[1])
            break;
    }

    int flag = msg_loop();
    debug("server thread end\n");
    return flag;
}
