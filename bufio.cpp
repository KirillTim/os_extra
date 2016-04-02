#include "bufio.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

buf_t::buf_t() {
    size = 0;
    data = NULL;
}

buf_t::~buf_t() {
    size = 0;
    free(data);
}

int buf_t::read_all(int fd) {
    const int BUF_SIZE = 512;
    char buf[BUF_SIZE];
    int len = 0;
    for (;;) {
        ssize_t count = read(fd, buf, BUF_SIZE);
        if (count < 0) {
            if (errno != EAGAIN)
                return -1;
            return len;
        }
        else if (count == 0) {
            return 0;
        }
        char* data_new = (char*)malloc(size+count);
        memcpy(data, data_new, size);
        memcpy(data+size, buf, (size_t)count);
        size += count;
        free(data);
        data = data_new;
    }
}
