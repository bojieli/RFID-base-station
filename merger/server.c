#include "merger.h"

static bool check_packet(unsigned char* pack) {
    unsigned char checksum = 0;
    int i;
    for (i=0; i<PACKET_SIZE; i++)
        checksum ^= pack[i];
    return (checksum == 0);
}

static void handle_student(char* id, int action) {
    const int state_table[][2] = {
        {1, 2},
        {1, 3},
        {4, 2},
        {1, 3},
        {4, 2},
    };
    int old_state = get(students, id);
    int new_state = state_table[old_state][action];
    debug_verbose("student %s at %s, state %d => %d", id, action ? "slave" : "master", old_state, new_state);
    set(students, id, new_state);
    clear_timeout(id);
    if (new_state != 0)
        set_timeout(id);
}

static void handle_packet(unsigned char* pack, int action) {
    if (!check_packet(pack)) {
        debug("Received invalid packet: ");
        print_buf(pack, PACKET_SIZE);
        return;
    }

    char* id = safe_malloc(ID_SIZE+1);
    int i;
    for (i=0; i<PACKET_SIZE; i++) {
        id[i*2] = tohexchar(pack[i] >> 4);
        id[i*2+1] = tohexchar(pack[i] & 0xF);
    }
    id[ID_SIZE] = '\0';
    handle_student(id, action);
    free(id);
}

static int msg_loop(int sockfd) {
    struct pollfd fds[3] = {
        {.fd = -1, .events = POLLIN}, // master 
        {.fd = -1, .events = POLLIN}, // slave
        {.fd = sockfd, .events = POLLIN} // accept connection
    };

    unsigned char* buf = safe_malloc(PACKET_SIZE); // packet buf

    while (1) {
        IF_ERROR(poll(fds, 3, -1), "poll")
        if (fds[0].revents & POLLERR || fds[1].revents & POLLERR || fds[2].revents & POLLERR) {
            fatal("Error in polling");
            return -1;
        }

        // accept connection
        if (fds[2].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t sin_size = sizeof(client_addr);
            int newfd;
            IF_ERROR((newfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)), "accept")
            char* client_addr_str = inet_ntoa(client_addr.sin_addr);
            if (0 == strcmp(client_addr_str, get_config("listen.local_ip")) ||
                0 == strcmp(client_addr_str, "127.0.0.1")) {
                fds[0].fd = newfd;
                report_it_now("master (%s) connected", client_addr_str);
            } else {
                fds[1].fd = newfd;
                report_it_now("slave (%s) connected", client_addr_str);
            }
        }

        // read from established connection
        int i;
        for (i=0; i<2; i++) {
            if (fds[i].revents & POLLIN) {
                int readlen = recvn(fds[i].fd, buf, PACKET_SIZE);
                if (readlen == -1) {
                    fatal("read from %s", i ? "slave" : "master");
                    return -1;
                }
                if (readlen < PACKET_SIZE) {
                    report_it_now(i ? "slave exit" : "master exit");
                    fds[i].fd = -1;
                    continue;
                }
                receiver_alive[i] = true;
                // ID with first byte 0x00 is for internal testing, drop it
                if (buf[0] != 0)
                    handle_packet(buf, i);
            }
        }
    }
}

int init_server()
{
    debug("server thread begin");

    int sockfd;
    struct sockaddr_in myaddr;
    IF_ERROR((sockfd = socket(AF_INET, SOCK_STREAM, 0)), "socket")
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(atoi(get_config("listen.port")));
    myaddr.sin_addr.s_addr = inet_addr(get_config("listen.host"));
    bzero(&(myaddr.sin_zero), 8);
    int yes = 1;
    IF_ERROR(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)), "setsockopt")
    IF_ERROR(bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)), "bind")
    IF_ERROR(listen(sockfd, 10), "listen")
    debug("listening %s:%s, local ip %s", get_config("listen.host"), get_config("listen.port"), get_config("listen.local_ip"));

    int flag = msg_loop(sockfd);

    debug("server thread end");
    return flag;
}
