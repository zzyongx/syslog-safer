/* vim:expandtab:shiftwidth=4:tabstop=4:smarttab:
 */

#include <cstdlib>
#include <time.h>
#include <ringbuffer.h>

RingBuffer::RingBuffer(size_t size, bool verbose, const char *notifyf)
{
    int eno = pthread_mutex_init(&mutex_, 0);
    if (eno != 0) throw eno;

    eno = pthread_cond_init(&cond_, 0);
    if (eno != 0) throw eno;

    size_   = size;
    buffer_ = (char *) malloc(size_);
    if (!buffer_) throw errno;

    quit_    = false;
    verbose_ = verbose;

    notifyf_ = notifyf;
}

RingBuffer::~RingBuffer()
{
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
    free(buffer_);
}

bool RingBuffer::ensureSpace(size_t n, bool *droped)
{
    bool hasSpace = false;
    if (droped) *droped = false;
    
    while (!hasSpace) {
        size_t qlen = queue_.size();
        if (qlen == 0) {
            hasSpace = true;
        } else {
            std::pair<size_t, size_t> range;
            if (qlen == 1) {
                range = queue_.front();
            } else {
                range.first  = queue_.front().first;
                range.second = queue_.back().second;
            }

            size_t used = (range.second > range.first) ?
                range.second - range.first :
                size_ + range.second - range.first;
            if (used + n > size_) {
                if (droped) *droped = true;
                queue_.pop_front();
            } else {
                hasSpace = true;
            }
        }
    }
    return true;
}

size_t RingBuffer::write(const char *buffer, size_t n)
{
    if (n > size_) return 0;
    pthread_mutex_lock(&mutex_);

    bool droped;
    ensureSpace(n, &droped);

    size_t qlen = queue_.size();
    if (qlen == 0) {
        queue_.push_back(std::make_pair(0, n));
        memcpy(buffer_, buffer, n);
    } else {
        std::pair<size_t, size_t> range = queue_.back();

        size_t first = range.second;
        size_t second = (first + n) % size_;
        if (first <= second) {
            memcpy(buffer_ + first, buffer, n);
        } else {
            memcpy(buffer_ + first, buffer, size_ - first);
            memcpy(buffer_, buffer + (size_ - first), second);
        }
        queue_.push_back(std::make_pair(first, second));
    }

    pthread_mutex_unlock(&mutex_);

    pthread_cond_signal(&cond_);

    if (droped && notifyf_) notify();
    if (verbose_) printf("PUSH %.*s", (int) n, buffer);

    return true;
}

size_t RingBuffer::read(char *buffer, size_t n)
{
    pthread_mutex_lock(&mutex_);
    if (!quit_ && queue_.empty()) {
        pthread_cond_wait(&cond_, &mutex_);
    }

    size_t nn = 0;
    while (!queue_.empty()) {
        std::pair<size_t, size_t> range = queue_.front();
        
        if (range.first <= range.second) {
            if (nn + range.second - range.first <= n) {
                memcpy(buffer + nn, buffer_ + range.first, range.second - range.first);
                nn += range.second - range.first;
            } else {
                break;
            }
        } else {
            if (nn + size_ + range.second - range.first <= n) {
                memcpy(buffer + nn,  buffer_ + range.first, size_ - range.first);
                memcpy(buffer + nn + size_ - range.first, buffer_, range.second);
                nn += size_ + range.second - range.first;
            } else {
                break;
            }
        }
        queue_.pop_front();
    }

    pthread_mutex_unlock(&mutex_);

    if (verbose_) printf("POP %.*s", (int) nn , buffer);

    return nn;
}

bool RingBuffer::interrupt()
{
    quit_ = true;
    pthread_cond_signal(&cond_);
    return true;
}

bool RingBuffer::notify() const
{
    FILE *fp = fopen(notifyf_, "w");
    if (fp) {
        fprintf(fp, "syslog-safer: DROP @%ld", (long) time(0));
        fclose(fp);
    }
    return true;
}
