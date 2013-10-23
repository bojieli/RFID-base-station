#include "merger.h"

static bool check_packet(unsigned char* pack) {
    unsigned char checksum = 0;
    int i;
    for (i=0; i<PACKET_SIZE; i++)
        checksum ^= pack[i];
    return (checksum == 0);
}

static void handle_student(char* id, bool action) {
    state2int converter;
    converter.i = get(students, id);
    student_state state = converter.s;

    if (state.tail_count) { // rotate tail
        state.tail = ((state.tail << 1) + action) & ((1<<TAIL_SAMPLE_LEN)-1);
        if (state.tail_count < TAIL_SAMPLE_LEN)
            ++state.tail_count;
    }
    else if (state.head_master_count + state.head_slave_count >= HEAD_SAMPLE_LEN) { // first action after head
        state.tail_count = 1;
        state.tail = action;
    }
    else { // in head
        action ? ++state.head_slave_count : ++state.head_master_count;
    }

    converter.s = state;
    set(students, id, converter.i);
    set_timeout(id);

    debug_verbose("student %s at %s head_master_count %d head_slave_count %d tail %x tail_count %d", id, action ? "slave" : "master", state.head_master_count, state.head_slave_count, state.tail, state.tail_count);
}

static void handle_packet(unsigned char* pack, int action) {
    if (atoi(get_config("student.checksum_enable")) && !check_packet(pack)) {
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

static char* get_local_ip() {
    int fd;
    struct ifreq ifr;
   
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
   
    return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
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
            char* client_addr_str = strdup(inet_ntoa(client_addr.sin_addr));
            if (0 == strcmp(client_addr_str, get_local_ip()) ||
                0 == strcmp(client_addr_str, "127.0.0.1")) {
                fds[0].fd = newfd;
                report_it_now("master (%s) connected", client_addr_str);
            } else {
                fds[1].fd = newfd;
                report_it_now("slave (%s) connected", client_addr_str);
            }
            free(client_addr_str);
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

                if (atoi(get_config("student.first_byte_zero_enable"))) {
                    int j;
                    for (j=0; j<PACKET_SIZE; j++)
                        if (buf[j] != 0) {
                            handle_packet(buf, i);
                            break;
                        }
                    // all bytes zero, it's heartbeat
                }
                else if (buf[0] != 0) {
                    // ID with first byte 0x00 is for internal testing, drop it
                    handle_packet(buf, i);
                }
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
