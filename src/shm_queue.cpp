#include "shm_queue.hpp"
//
#include <chrono>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <coda/error.hpp>

static int64_t clock_get_milliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

namespace nnxcam {

ShmQueue::ShmQueue(const std::string& key_fname, size_t queue_size)
{

}

void ShmQueue::connect_shm(const char* key_fname, size_t q_size)
{
    if (_data)
    {
        throw coda_error("ShmQueue: queue already connected");
    }
    key_t key;
    if ((key_t) -1 == (key = ftok(key_fname, 1)))
    {
        throw coda_error("ShmQueue: error creating key from file '%s': %s", key_fname, coda_strerror(errno));
    }
    bool created = false;
    if (-1 == (_shm_id = shmget(key, q_size + sizeof(_queue_info), 0666)))
    {
        if (-1 == (_shm_id = shmget(key, q_size + sizeof(_queue_info), IPC_CREAT | 0666))) // not created? creating...
        {
            throw coda_error("ShmQueue: error creating queue shm: %s", coda_strerror(errno));
        }
        created = true;
    }
    uint8_t *ptr = (uint8_t*)shmat(_shm_id, 0, 0);
    if ((void*)-1 == ptr)
    {
        throw coda_error("ShmQueue: error attaching queue shmem: %s", coda_strerror(errno));
    }
    _queue_info = (ShmQueueInfo*)ptr;
    if (created)
    {
        memset(ptr, 0, q_size + sizeof(_queue_info));
    }
    ptr += sizeof(ShmQueueInfo);
    _data = ptr;
}

void ShmQueue::lock_queue()
{
    Mutex(&_queue_info->atom).lock();
}

void ShmQueue::unlock_queue()
{
    Mutex(&_queue_info->atom).unlock();
}

void ShmQueue::notify_not_full()
{
    uint64_t data = 1;
    write(_q_not_full_fd, &data, sizeof(uint64_t));
}

void ShmQueue::notify_not_empty()
{
    uint64_t data = 1;
    write(_q_not_empty_fd, &data, sizeof(data));
}

void ShmQueue::read_event_fd(int fd)
{
    uint64_t data;
    int ret = read(fd, &data, sizeof(data));
    if (ret < 0)
    {
        // error
    }
}

bool ShmQueue::wait_fd_data_available(int fd, int timeout)
{
    const int MAX_NUM_EVENTS = 32;
    struct epoll_event events[MAX_NUM_EVENTS];

    int64_t start_time = clock_get_milliseconds();
    int elapsed = 0;
    while (true)
    {
        int _timeout = timeout == -1 ? -1 : std::max(0, timeout - elapsed);
        int nfds = epoll_wait(_epoll_fd, events, MAX_NUM_EVENTS, _timeout);
        for (int i = 0; i < nfds; i++)
        {
            if (events[i].events & EPOLLIN && events[i].data.fd == fd)
            {
                return true;
            }
        }
        if (timeout == -1)
        {
            continue;
        }
        elapsed = clock_get_milliseconds() - start_time;
        if (elapsed <= 0)
        {
            break;
        }
    }
    return false;
}

ShmQueue::ShmQueue(int fd, int can_consume_fd, int can_produce_fd,
                   size_t size, bool init) :
    _fd(fd),
    _q_not_empty_fd(can_consume_fd),
    _q_not_full_fd(can_produce_fd)
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
    set_blocking(_q_not_empty_fd, false);
    set_blocking(_q_not_full_fd, false);
}

}
