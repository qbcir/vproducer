#ifndef __NNXCAM_VPROD_SHM_QUEUE_HPP_17758__
#define __NNXCAM_VPROD_SHM_QUEUE_HPP_17758__

#include <cstddef>
#include <cstring>
#include <string>
#include <sys/time.h>
#include <time.h>

namespace nnxcam {

class ShmLock
{
public:
    ShmLock(int *futexp);
    ~ShmLock();
private:
    int *_f = nullptr;
};

struct ShmHeader
{
    size_t length;
    int futexp;
    size_t start;
    size_t end;
};

class ShmQueue
{
public:
    static ShmQueue* connect(const std::string &sock_path);

    ShmQueue(int fd, int can_consume_fd, int can_produce_fd, size_t size, bool init);

    char *dequeue(size_t *length, struct timespec *ts=NULL);
    int enqueue(const char *memory, size_t length, struct timespec *ts=NULL);
    size_t used();

private:
    inline void* get_addr(size_t offset)
    {
        return ((char*)_header + offset + sizeof(ShmHeader));
    }
    inline void* get_head()
    {
        return get_addr(0);
    }
    inline void* get_start()
    {
        return get_addr(_header->start);
    }
    inline void* get_end()
    {
        return get_addr(_header->end);
    }

    int pop(char *data, size_t length);
    int push(const char *data, size_t length);
    size_t reserved();

    bool set_blocking(int fd, bool blocking);
    bool fd_wait_read(int fd, struct timespec* ts);
    void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result);

    ShmHeader *_header = nullptr;
    int _fd;
    int _can_consume_fd;
    int _can_produce_fd;
};

}

#endif /* __NNXCAM_VPROD_SHM_QUEUE_HPP_17758__ */
