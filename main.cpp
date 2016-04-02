#include <iostream>
#include <netdb.h>

using namespace std;

int make_server(char* port) {
    const int LISTEN_QUEUE = 100;
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
    if(listen(sock, LISTEN_QUEUE)) {
        return -4;
    }
    freeaddrinfo(localhost);
    return sock;
}

int main(int argc, char** argv) {
    return 0;
}
