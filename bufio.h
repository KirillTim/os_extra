#ifndef OS_EXTRA_BUFIO_H
#define OS_EXTRA_BUFIO_H

#include <sys/types.h>

struct buf_t {
public:
    buf_t();
    ~buf_t();
    size_t size;
    char* data;
    //read from non-block socket
    int read_all(int fd);
};

#endif //OS_EXTRA_BUFIO_H
