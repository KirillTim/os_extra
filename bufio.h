#ifndef OS_EXTRA_BUFIO_H
#define OS_EXTRA_BUFIO_H

#include <sys/types.h>

struct buf_t {
public:
    buf_t();
    ~buf_t();

    size_t size;
    char* data;

    /*read from non-block socket.
     return -1 on error, 0 on file close , bytes read otherwise */
    int read_all(int fd);
};

#endif //OS_EXTRA_BUFIO_H
