/* vim:expandtab:shiftwidth=4:tabstop=4:smarttab:
 */

#ifndef _LOGWRITER_H_
#define _LOGWRITER_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

template <typename InputBuffer>
class LogWriter {
public:
    LogWriter(const char *dst, InputBuffer *inbuffer, bool isStream = false);
    ~LogWriter();

    bool run();
    bool stop();

private:
    static int open(const char *addr, bool isStream); 

private:
    bool         isStream_;
    const char  *dst_;
    InputBuffer *inbuffer_;
    int          fd_;
    char         buffer_[InputBuffer::nbuffer];
    bool         quit_;
};

template <typename InputBuffer>
LogWriter<InputBuffer>::LogWriter(const char *dst, InputBuffer *inbuffer, bool isStream)
    : isStream_(isStream), dst_(dst), inbuffer_(inbuffer), fd_(-1), quit_(false) { }

template <typename InputBuffer>
LogWriter<InputBuffer>::~LogWriter() 
{
    if (fd_ != -1) close(fd_);
}

template <typename InputBuffer>
int LogWriter<InputBuffer>::open(const char *dst, bool isStream)
{
    int fd;

    if ((fd = socket(AF_UNIX, isStream ? SOCK_STREAM : SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "socket() error, %d:%s\n", errno, strerror(errno));
        return -1;
    }

    struct sockaddr_un un;
    socklen_t len;

    memset(&un, 0x00, sizeof(un));
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, "/var/tmp/%05d", getpid());
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

    unlink(un.sun_path);

    /* unlink tmp file, the file will not be really unlinked until closed */
    if (bind(fd, (struct sockaddr *)(&un), len) < 0) {
        fprintf(stderr, "bind(%s) error, %d:%s\n", un.sun_path, errno, strerror(errno));
        unlink(un.sun_path);
        close(fd);
        return -1;
    }
    unlink(un.sun_path);

    memset(&un, 0x00, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, dst);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(dst);

    if (connect(fd, (struct sockaddr*)(&un), len) < 0) {
        fprintf(stderr, "connect(%s) error, %d:%s\n", dst, errno, strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

template <typename InputBuffer>
bool LogWriter<InputBuffer>::run()
{
    while (!quit_) {
        int fd = open(dst_, isStream_);
        if (fd == -1) {
            sleep(1);
            continue;
        }

        while (!quit_) {
            size_t n = inbuffer_->read(buffer_, InputBuffer::nbuffer);
            size_t pos = 0;
            ssize_t nn = 0;
            while((nn = send(fd, buffer_ + pos, n - pos, MSG_NOSIGNAL)) > 0) {
                pos += nn;
            }
            if (nn == -1 && errno != EAGAIN) break;
        }

        close(fd);
    }

    return true;
}

template <typename InputBuffer>
bool LogWriter<InputBuffer>::stop()
{
    quit_ = true;
    return true;
}

#endif
