#include <iostream>
#include <netdb.h>
#include <fcntl.h>
#include <unordered_map>

#include <sys/epoll.h>
#include <zconf.h>
#include <vector>
#include "bufio.h"
#include "parsers.h"
#include "run_piped.h"

using namespace std;

int make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags);
}

int make_socket_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    flags &= ~O_NONBLOCK;
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

unordered_map<int, string> buffers;

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
    buffers.insert(make_pair(client, ""));
    return client;
}

int start_prog(vector<execargs_t> prog, int fd) {
    make_socket_blocking(fd);
    int pid = fork();
    if (pid < 0) {
        return -1;
    } else if (pid) {
        cerr<<"chld: "<<pid<<"\n";
        close(fd);//only child handle descriptor now
        //ignore child, they will finish somehow
        return 0;
    } else {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        int res = run_piped(prog, fd);
        close(fd);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        exit(res);
    }
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

    /*while (1) {
        int n = epoll_wait(efd, events, (int) events_size, -1);
        for (int i = 0; i < n; i++) {
            int cur_fd = events[i].data.fd;
            if (server == cur_fd) {
                epoll_ctl(efd, EPOLL_CTL_DEL, cur_fd, &events[i]);
                int res = accept(server, 0, 0);
                //make_socket_non_blocking(res);
                cerr<<"connected: "<<res<<"\n";
                dup2(res, STDIN_FILENO);
                dup2(res, STDOUT_FILENO);
                char* p1[3];
                p1[0] = (char*)"cat";
                p1[1] = (char*)"/proc/cpuinfo";
                p1[2] = 0;
                char* p2[3];
                p2[0] = (char*)"grep";
                p2[1] = (char*)"model name";
                p2[2] = 0;
                char* p3[4];
                p3[0] = (char*)"head";
                p3[1] = (char*)"-n";
                p3[2] = (char*)"1";
                p3[3] = 0;
                vector<execargs_t > v({p1,p2,p3});
                run_piped(v, -1);
                return 0;
            }
        }
    }*/

    while (1) {
        if (events_new_size != events_size) {
            events_size = events_new_size;
            events = (epoll_event *) calloc(events_size, sizeof(event));
        }
        int n = epoll_wait(efd, events, (int) events_size, -1);
        for (int i = 0; i < n; i++) {
            int cur_fd = events[i].data.fd;
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
                || (events[i].events & EPOLLRDHUP)) {
                cerr << "error on socket: " << cur_fd << "\n";
                close(cur_fd);
                continue;
            }
            else if (server == cur_fd) {
                int res = accept(server, 0, 0);
                res = add_client(efd, res);
                if (res < 0)
                    return -res;
                else
                    events_new_size++;
                continue;
            }
            else {
                char buf[1];
                bool read_finished = false;
                while (1) {
                    ssize_t count = read(cur_fd, buf, 1);
                    if (count == 0) {
                        cerr << "on socket " << cur_fd << " close\n";
                        close(cur_fd);
                        break;
                    }
                    else if (count == -1 && errno == EAGAIN) {
                        cerr << "nothing more to read on: " << cur_fd << "\n";
                        break;
                    }
                    else {
                        buffers.find(cur_fd)->second += buf[0];
                        if (buf[0] == '\n') {
                            read_finished = true;
                            break;
                        }
                    }
                }
                if (read_finished) {
                    cerr<<"command: "<<buffers.find(cur_fd)->second<<"\n";
                    vector<execargs_t> prog;
                    int res = parse_command((char *) buffers.find(cur_fd)->second.c_str(),
                                            buffers.find(cur_fd)->second.size(), prog);
                    if (res == 1) {
                        epoll_ctl(efd, EPOLL_CTL_DEL, cur_fd, &events[i]);
                        buffers.erase(buffers.find(cur_fd));
                        events_new_size--;
                        start_prog(prog, cur_fd);
                    } else {
                        cerr << "can't parse program on sock " << cur_fd << "\n";
                        close(cur_fd);
                        epoll_ctl(efd, EPOLL_CTL_DEL, cur_fd, &events[i]);
                        events_new_size--;
                    }
                }

                /*else {
                    cerr << "on socket " << cur_fd << " read " << count << " bytes, buf: "<<buf->second.data<<"\n";
                }*/
                //read one byte at time, find "\n", direct the rest input to run_piped
                //don't want to deal with rest of the line after '\n'

                /*vector<program> programs;
                int res = parse_command(buf->second.data, buf->second.size, programs);
                if (res == 1) { //success
                    cerr<<"command parsed, remove event\n";
                    epoll_ctl(efd, EPOLL_CTL_DEL, cur_fd, &events[i]);
                    events_new_size ++;
                    run_piped(programs, cur_fd);
                } else if (res == -1) { //error, close socket and remove
                    cerr<<"bad string as command, close socket, remove event\n";
                    close(cur_fd);
                    epoll_ctl(efd, EPOLL_CTL_DEL, cur_fd, &events[i]);
                    events_new_size ++;
                }*/
            }
        }
    }

    free(events);

    close(server);

    return EXIT_SUCCESS;

    return 0;
}
