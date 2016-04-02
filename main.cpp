#include <iostream>
#include <netdb.h>
#include <fcntl.h>
#include <unordered_map>

#include <sys/epoll.h>
#include <zconf.h>
#include "bufio.h"

using namespace std;

int make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags);
}

int make_server(char *port) {
    struct addrinfo *localhost;
    if (getaddrinfo("0.0.0.0", port, 0, &localhost)) {
        return -1;
    }
    int sock = socket(localhost->ai_family, SOCK_STREAM, 0);
    if (sock < 0) {
        return -2;
    }
    if (bind(sock, localhost->ai_addr, localhost->ai_addrlen)) {
        return -3;
    }
    if (make_socket_non_blocking(sock) < 0) {
        return -4;
    }
    if (listen(sock, SOMAXCONN)) {
        return -5;
    }
    freeaddrinfo(localhost);
    return sock;
}

unordered_map<int, buf_t> buffers;

int add_client(int efd, int client) {
    cerr << "new client: " << client << "\n";
    if (client < 0)
        return -11;
    if (make_socket_non_blocking(client) < 0)
        return -12;
    struct epoll_event event;
    event.data.fd = client;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, client, &event) < 0)
        return -13;
    buffers.insert(make_pair(client, buf_t()));
    return client;
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        return 1;
    }
    int server = make_server(argv[1]);
    if (server < 0)
        return 2;

    int efd = epoll_create1(0);
    if (efd == -1) {
        return 3;
    }

    struct epoll_event event;
    struct epoll_event *events;
    size_t events_size = 1, events_new_size = 1;

    event.data.fd = server;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, server, &event) < 0)
        return 4;

    /* Buffer where events are returned */
    events = (epoll_event *) calloc(events_size, sizeof(event));

    while (1) {
        if (events_new_size != events_size) {
            events_size = events_new_size;
            events = (epoll_event *) calloc(events_size, sizeof(event));
        }
        int n, i;
        n = epoll_wait(efd, events, (int) events_size, -1);
        for (i = 0; i < n; i++) {
            int cur_fd = events[i].data.fd;
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
                || (events[i].events & EPOLLRDHUP)) {
                cerr << "error on socket: " << cur_fd << "\n";
                close(cur_fd);
                continue;
            }
            else if (server == cur_fd) {
                /* We have a notification on the listening socket, which
                   means one or more incoming connections. */
                int res = accept(server, 0, 0);
                res = add_client(efd, res);
                if (res < 0)
                    return -res;
                continue;
            }
            else {
                buf_t buf = buffers.find(cur_fd)->second;
                int count = buf.read_all(cur_fd);
                if (count == 0) {
                    cerr << "on socket " << cur_fd << " close\n";
                    close(cur_fd);
                }
                else {
                    cerr << "on socket " << cur_fd << " read " << count << " bytes\n";
                }
            }
        }
    }

    free(events);

    close(server);

    return EXIT_SUCCESS;

    return 0;
}
