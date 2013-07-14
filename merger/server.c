#include "common.h"

static int packet_size;

static bool check_packet(char* no) {
    unsigned char checksum = 0;
    int i;
    for (i=0; i<packet_size; i++)
        checksum ^= no[i];
    return (checksum == 0);
}

static void handle_packet(char* no, int action) {
    if (!check_packet(no))
        return;
    const int state_table[][2] = {
        {1, 2},
        {1, 3},
        {4, 2},
        {1, 3},
        {4, 2},
    };
    int new_state = state_table[get(students, no)][action];
    set(students, no, new_state);
    clear_timeout(no);
    if (new_state != 0)
        set_timeout(no);
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
                char* buf = malloc(packet_size);
                int readlen = recvn(clientfds[i], buf, packet_size);
                if (readlen == -1) {
                    fprintf(stderr, "Error: read from %s\n", i ? "slave" : "master");
                    return -1;
                }
                if (readlen == 0) {
                    debug(i ? "slave exit\n" : "master exit\n");
                    return 0;
                }
                handle_packet(buf, i);
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
        if (client_addr.sin_addr.s_addr == htons(inet_addr(get_config("listen.local_ip")))) {
            clientfds[0] = newfd;
            debug("master connected\n");
        } else {
            clientfds[1] = newfd;
            debug("slave connected\n");
        }
        if (clientfds[0] && clientfds[1])
            break;
    }

    // packet_size is a static global variable to improve performance
    packet_size = atoi(get_config("student.packet_size"));
    if (packet_size < 1) {
        printf("Invalid config: student.packet_size");
        return 1;
    }

    int flag = msg_loop();
    debug("server thread end\n");
    return flag;
}
