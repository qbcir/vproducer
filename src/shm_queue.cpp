#include "shm_queue.hpp"
//
#include <fcntl.h>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/futex.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/select.h>

static int futex(int *uaddr, int futex_op, int val,
                 const struct timespec *timeout,
                 int *uaddr2, int val3)
{
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr, val3);
}

static void fwait(int *futexp)
{
    while (1)
    {
        if (__sync_bool_compare_and_swap(futexp, 1, 0))
        {
            break;
        }
        auto s = futex(futexp, FUTEX_WAIT, 0, NULL, NULL, 0);
        if (s == -1 && errno != EAGAIN)
        {
            throw std::runtime_error("futex-FUTEX_WAIT");
        }
    }
}

static void fpost(int *futexp)
{
    if (__sync_bool_compare_and_swap(futexp, 0, 1))
    {
        int s = futex(futexp, FUTEX_WAKE, 1, NULL, NULL, 0);
        if (s == -1)
        {
            throw std::runtime_error("futex-FUTEX_WAKE");
        }
    }
}

namespace nnxcam {

ShmLock::ShmLock(int *futexp) :
    _f(futexp)
{
    fwait(_f);
}

ShmLock::~ShmLock()
{
    fpost(_f);
}

ShmQueue::ShmQueue(int fd, int can_consume_fd, int can_produce_fd,
                   size_t size, bool init) :
    _fd(fd),
    _can_consume_fd(can_consume_fd),
    _can_produce_fd(can_produce_fd)
{
    size_t map_size = size + sizeof(ShmHeader);
    auto ptr = mmap(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    if (ptr == MAP_FAILED)
    {
        //errExit("map failed");
    }
    _header = (ShmHeader*)(ptr);
    if (init)
    {
        _header->length = size;
        _header->futexp = 1;
        _header->start = 0;
        _header->end = 0;
    }
    set_blocking(_can_consume_fd, false);
    set_blocking(_can_produce_fd, false);
}

size_t ShmQueue::reserved()
{
    if (_header->end >= _header->start)
    {
        return _header->length - _header->end + _header->start;
    } else
    {
        return _header->start - _header->end;
    }
}

int ShmQueue::push(const char *data, size_t length)
{
    if (_header->end >= _header->start)
    {
        size_t tail_len = _header->length - _header->end;
        if (tail_len > length)
        {
            memcpy(get_end(), data, length);
            _header->end += length;
        }
        else
        {
            size_t start_len = length - tail_len;
            memcpy(get_end(), data, tail_len);
            memcpy(get_head(), data + tail_len, start_len);
            _header->end = start_len;
        }
    }
    else
    {
        memcpy(get_end(), data, length);
        _header->end += length;
    }
    return 0;
}

int ShmQueue::enqueue(const char *data, size_t length, struct timespec *ts)
{
    ShmLock lock(&_header->futexp);
    while (length + sizeof(size_t) >= reserved())
    {
        int64_t cnt = 0;
        fpost(&_header->futexp);
        bool ret = fd_wait_read(_can_produce_fd, ts);
        fwait(&_header->futexp);
        if (!ret)
        {
            return -1;
        }
        read(_can_produce_fd, &cnt, sizeof(cnt));
    }
    push((char *)&length, sizeof(size_t));
    push(data, length);
    int64_t cnt = 1;
    write(_can_consume_fd, &cnt, sizeof(cnt));
    return 0;
}

int ShmQueue::pop(char* data, size_t length)
{
    if (_header->end < _header->start)
    {
        size_t head_len = _header->length - _header->start;
        if (head_len >= length)
        {
            memcpy(data, get_start(), length);
            _header->start += length;
        }
        else
        {
            size_t start_len = length - head_len;
            memcpy(data, get_start(), head_len);
            memcpy(data + head_len, get_head(), start_len);
            _header->start = start_len;
        }
    }
    else
    {
        memcpy(data, get_start(), length);
        _header->start += length;
    }
    return 0;
}

char* ShmQueue::dequeue(size_t *length, struct timespec *ts)
{
    ShmLock lock(&_header->futexp);
    while ((_header->start - _header->end + _header->length) % _header->length == 0)
    {
        int64_t cnt = 0;
        fpost(&_header->futexp);
        bool ret = fd_wait_read(_can_consume_fd, ts);
        fwait(&_header->futexp);
        if (!ret)
        {
            return nullptr;
        }
        read(_can_consume_fd, &cnt, sizeof(cnt));
    }
    pop((char*)(length), sizeof(size_t));
    //FIXME
    char *ptr = (char*)malloc(*length);
    pop(ptr, *length);
    int64_t cnt = 1;
    write(_can_produce_fd, &cnt, sizeof(cnt));
    return ptr;
}

size_t ShmQueue::used()
{
    ShmLock lock(&_header->futexp);
    return _header->length - reserved();
}

bool ShmQueue::fd_wait_read(int fd, struct timespec* ts)
{
    struct timespec start, end, intval, realint;
    struct timeval tv, *ptv = &tv;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while(true)
    {
        if (ts)
        {
            clock_gettime(CLOCK_MONOTONIC, &end);
            timespec_diff(&start, &end, &intval);
            timespec_diff(&intval, ts, &realint);
            if (realint.tv_sec < 0 || realint.tv_nsec < 0)
            {
                return false;
            }
            TIMESPEC_TO_TIMEVAL(ptv, &realint);
        }
        else
        {
            ptv = NULL;
        }
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        int ret = select(fd+1, &read_fds, NULL, NULL, ptv);
        if((ret > 0) && FD_ISSET(fd, &read_fds))
        {
            return true;
        }
    }
}

bool ShmQueue::set_blocking(int fd, bool blocking)
{
    if (fd < 0)
    {
        return false;
    }
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        return false;
    }
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}

void ShmQueue::timespec_diff(
        struct timespec *start,
        struct timespec *stop,
        struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0)
    {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    }
    else
    {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
}

}
