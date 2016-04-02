#include <iostream>
#include <netdb.h>
#include <fcntl.h>

#include <sys/epoll.h>

using namespace std;

int make_socket_non_blocking(int fd) {
    int flags = fcntl (fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    flags |= O_NONBLOCK;
    return fcntl (fd, F_SETFL, flags);
}

int make_server(char* port) {
    struct addrinfo *localhost;
    if(getaddrinfo("0.0.0.0", port, 0, &localhost)) {
        return -1;
    }
    int sock = socket(localhost->ai_family, SOCK_STREAM, 0);
    if(sock < 0) {
        return -2;
    }
    if(bind(sock, localhost->ai_addr, localhost->ai_addrlen)) {
        return -3;
    }
    if (make_socket_non_blocking(sock) < 0) {
        return -4;
    }
    if(listen(sock, SOMAXCONN)) {
        return -4;
    }
    freeaddrinfo(localhost);
    return sock;
}


int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf (stderr, "Usage: %s [port]\n", argv[0]);
        return 1;
    }
    int server = make_server(argv[1]);
    if (server < 0)
        return 2;

    int efd;
    struct epoll_event event;
    struct epoll_event *events;
    size_t events_size = 1;


    efd = epoll_create1(0);
    if (efd == -1) {
        return 3;
    }

    event.data.fd = server;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl (efd, EPOLL_CTL_ADD, server, &event) < 0)
        return 4;

    /* Buffer where events are returned */
    events = (epoll_event*)calloc (events_size, sizeof(event));


    return 0;
}
