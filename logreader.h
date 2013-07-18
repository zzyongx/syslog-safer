/* vim:expandtab:shiftwidth=4:tabstop=4:smarttab:
 */

#ifndef _LOGREADER_H_
#define _LOGREADER_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

template <typename OutputBuffer>
class LogReader {
public:
    LogReader(const char *src, OutputBuffer *outbuffer, bool isStream = false);
    ~LogReader();

    bool run();
    bool stop();

private:
    static int createStreamFd(const char *addr);
    static bool addStreamFd(int efd, int sfd, OutputBuffer *outbuffer);

    static int createDgramFd(const char *addr);
    static bool addDgramFd(int efd, int dfd, OutputBuffer *outbuffer);

private:
    bool          isStream_;
    const char   *src_;
    OutputBuffer *outbuffer_;
    int efd_;
    int sfd_;
    int dfd_;

    bool quit_;
};

template <typename OutputBuffer>
class EventProcessor {
public:
    enum FdType { Stream, Dgram, Normal };

    EventProcessor(int fd, int efd, OutputBuffer *outbuffer, FdType type = Normal)
        : fd_(fd), efd_(efd), outbuffer_(outbuffer), fdType_(type) {
        buffer_ = new char[OutputBuffer::nbuffer];
    }
    ~EventProcessor() {
        close(fd_);
        delete[] buffer_;
    }

    bool process();

private:
    int           fd_;
    int           efd_;
    OutputBuffer *outbuffer_;
    FdType        fdType_;
    char         *buffer_;
};

template <typename OutputBuffer>
bool EventProcessor<OutputBuffer>::process()
{
    if (fdType_ == Stream) {
        int fd = accept(fd_, 0, 0);
        if (fd > 0) {
            int flags = 1;
            if (ioctl(fd, FIONBIO, &flags) != 0) {
                fprintf(stderr, "ioctl(FIONBIO) error, %d:%s\n", errno, strerror(errno));
            }

            EventProcessor *ep = new EventProcessor(fd, efd_, outbuffer_);

            struct epoll_event eevent;
            eevent.events = EPOLLIN;
            eevent.data.ptr = ep;

            if (epoll_ctl(efd_, EPOLL_CTL_ADD, fd, &eevent) != 0) {
                delete ep;
                return false;
            }
            return true;
        }
    } else if (fdType_ == Dgram) {
        ssize_t nn;
        while ((nn = recv(fd_, buffer_, OutputBuffer::nbuffer, 0)) > 0) {
            outbuffer_->write(buffer_, nn);
        }
        return true;
    } else {
        ssize_t nn;
        while ((nn = recv(fd_, buffer_, OutputBuffer::nbuffer, 0)) > 0) {
            outbuffer_->write(buffer_, nn);
        }

        if (nn == 0 || (nn == -1 && errno != EAGAIN)) {
            delete this;
            return nn == 0;
        }
        return true;
    }
    return true;
}

template <typename OutputBuffer>
LogReader<OutputBuffer>::LogReader(const char *src, OutputBuffer *outbuffer, bool isStream)
    : isStream_(isStream), src_(src), outbuffer_(outbuffer),
      efd_(-1), sfd_(-1), dfd_(-1), quit_(false) { }

template <typename OutputBuffer>
LogReader<OutputBuffer>::~LogReader()
{
    if (efd_ != -1) close(efd_);
    if (sfd_ != -1) close(sfd_);
    if (dfd_ != -1) close(dfd_);
}

template <typename OutputBuffer>
int LogReader<OutputBuffer>::createStreamFd(const char *addr)
{
    int fd;
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket() error, %d:%s\n", errno, strerror(errno));
        return -1;
    }

    int flags = 1;
    if (ioctl(fd, FIONBIO, &flags) != 0) {
        fprintf(stderr, "ioctl(FIONBIO) error, %d:%s\n", errno, strerror(errno));
        close(fd);
        return -1;
    }

    struct sockaddr_un un;
    socklen_t len;

    memset(&un, 0x00, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, addr);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr);

    unlink(addr);

    if (bind(fd, (struct sockaddr*)(&un), len) < 0) {
        fprintf(stderr, "bind(%s) error, %d:%s\n", addr, errno, strerror(errno));
        close(fd);
        return -1;
    }

    if (listen(fd, 1024) < 0) {
        fprintf(stderr, "listen() error, %d:%s\n", errno, strerror(errno));
        close(fd);
        return -1;
    }

    if (chmod(addr, 0666) != 0) {
        fprintf(stderr, "chmod(%s, 0666) error, %d:%s\n", addr, errno, strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

template <typename OutputBuffer>
bool LogReader<OutputBuffer>::addStreamFd(int efd, int sfd, OutputBuffer *outbuffer)
{
    EventProcessor<OutputBuffer> *ep =
        new EventProcessor<OutputBuffer>(sfd, efd, outbuffer, EventProcessor<OutputBuffer>::Stream);

    struct epoll_event eevent;
    eevent.events = EPOLLIN;
    eevent.data.ptr = ep;

    return epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &eevent) == 0;
}

template <typename OutputBuffer>
int LogReader<OutputBuffer>::createDgramFd(const char *addr)
{
    int fd;
    if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "socket() error, %d:%s\n", errno, strerror(errno));
        return -1;
    }

    int flags = 1;
    if (ioctl(fd, FIONBIO, &flags) != 0) {
        fprintf(stderr, "ioctl(FIONBIO) error, %d:%s\n", errno, strerror(errno));
        close(fd);
        return -1;
    }

    struct sockaddr_un un;
    socklen_t len;

    memset(&un, 0x00, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, addr);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr);

    unlink(addr);

    if (bind(fd, (struct sockaddr*)(&un), len) < 0) {
        fprintf(stderr, "bind(%s) error, %d:%s\n", addr, errno, strerror(errno));
        close(fd);
        return -1;
    }

    if (chmod(addr, 0666) != 0) {
        fprintf(stderr, "chmod(%s, 0666) error, %d:%s\n", addr, errno, strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

template <typename OutputBuffer>
bool LogReader<OutputBuffer>::addDgramFd(int efd, int dfd, OutputBuffer *outbuffer)
{
    EventProcessor<OutputBuffer> *ep =
        new EventProcessor<OutputBuffer>(dfd, efd, outbuffer, EventProcessor<OutputBuffer>::Dgram);

    struct epoll_event eevent;
    eevent.events = EPOLLIN;
    eevent.data.ptr = ep;

    return epoll_ctl(efd, EPOLL_CTL_ADD, dfd, &eevent) == 0;
}

template <typename OutputBuffer>
bool LogReader<OutputBuffer>::run()
{
    size_t nevent = 1024;
    efd_ = epoll_create(nevent);
    if (efd_ == -1) {
        fprintf(stderr, "epoll_create() error, %d:%s\n", errno, strerror(errno));
        return false;
    }

    struct epoll_event *events =
        (struct epoll_event *) calloc(nevent, sizeof(struct epoll_event));

    if (isStream_) {
        sfd_ = createStreamFd(src_);
        if (sfd_ == -1) return false;

        if (!addStreamFd(efd_, sfd_, outbuffer_)) return false;
    } else {
        dfd_ = createDgramFd(src_);
        if (dfd_ == -1) return false;

        if (!addDgramFd(efd_, dfd_, outbuffer_)) return false;
    }

    while (!quit_) {
        int n = epoll_wait(efd_, events, nevent, 500);

        if (n == -1) {
            if (errno == EINTR) continue;
            else break;
        }

        for (int i = 0; i < n; ++i) {
            EventProcessor<OutputBuffer> *ep = (EventProcessor<OutputBuffer> *) events[i].data.ptr;
            ep->process();
        }
    }
    return true;
}

template <typename OutputBuffer>
bool LogReader<OutputBuffer>::stop()
{
    quit_ = true;
    return true;
}

#endif
