/* vim:expandtab:shiftwidth=4:tabstop=4:smarttab:
 */

#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include <cstdio>
#include <cstring>
#include <deque>
#include <errno.h>
#include <pthread.h>

class RingBuffer {
public:
    RingBuffer(size_t size, bool verbose = false, const char *notifyf = 0);
    ~RingBuffer();

    size_t write(const char *buffer, size_t n);
    size_t read(char *buffer, size_t n);
    bool interrupt();

    static const size_t nbuffer = 16384; // 16K

private:
    bool ensureSpace(size_t n, bool *droped = 0);
    bool notify() const;

private:
    size_t size_;
    char *buffer_;

    std::deque< std::pair<size_t, size_t> > queue_;

    pthread_mutex_t mutex_;
    pthread_cond_t  cond_;
    bool            quit_;

    bool verbose_;
    const char *notifyf_;
};

#endif
