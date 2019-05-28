#ifndef __NNXCAM_VPROD_SHM_QUEUE_HPP_17758__
#define __NNXCAM_VPROD_SHM_QUEUE_HPP_17758__

#include <atomic>
#include <cstddef>
#include <cstring>
#include <vector>
#include <string>
#include <sys/time.h>
#include <time.h>

#include "shm_mutex.hpp"

namespace nnxcam {

struct Serializable
{
    Serializable() {}
    virtual ~Serializable() {}
    virtual bool serialize(void *p) const = 0;
    virtual bool deserialize(const void *p) = 0;
    virtual size_t size() const { return 0; }
};

struct ShmQueueInfo
{
    std::atomic<int32_t> atom;
    uint32_t size;
    uint32_t capacity;
    uint32_t head;
    uint32_t tail;
};

class ShmQueue
{
public:
    ShmQueue(const std::string& key_fname, size_t queue_size);

    template<typename T>
    bool put(const T& obj, int timeout = -1)
    {
        lock_queue();
        try
        {
            while (_queue_info->size + obj.size() >= _queue_info->capacity)
            {
                unlock_queue();
                if (!wait_fd_data_available(_q_not_full_fd, timeout))
                {
                    return false;
                }
                lock_queue();
                read_event_fd(_q_not_full_fd);
            }
            _serialize(obj);
            notify_not_empty();
            unlock_queue();
        }
        catch (...)
        {
            unlock_queue();
            throw;
        }
        return true;
    }

    template<typename T>
    bool get(T& obj, int timeout = -1)
    {
         lock_queue();
         try
         {
             while (_queue_info->size < obj.size())
             {
                 unlock_queue();
                 if (!wait_fd_data_available(_q_not_empty_fd, timeout))
                 {
                     return false;
                 }
                 lock_queue();
                 read_event_fd(_q_not_empty_fd);
             }
             _deserialize(obj);
             notify_not_full();
             unlock_queue();
         }
         catch(...)
         {
             unlock_queue();
             throw;
         }
         return true;
    }

private:
    void connect_shm(const char* key_fname, size_t q_size);

    template<typename T>
    void _serialize(const T& obj)
    {
        size_t obj_sz = obj.size();
        if (_queue_info->tail >= _queue_info->head)
        {
            size_t tail_sz = _queue_info->capacity - _queue_info->tail;
            if (tail_sz <= obj_sz)
            {
                uint8_t tmp_buf[obj_sz];
                obj.serialize(tmp_buf);
                size_t head_sz = obj_sz - tail_sz;
                memcpy(&_data[_queue_info->tail], tmp_buf, tail_sz);
                memcpy(_data, tmp_buf + tail_sz, head_sz);
                _queue_info->tail = head_sz;
            }
        }
        else
        {
            obj.serialize(&_data[_queue_info->tail]);
            _queue_info->tail += obj_sz;
        }
        _queue_info->size += obj_sz;
    }

    template<typename T>
    void _deserialize(T& obj)
    {
        size_t obj_sz = obj.size();
        if (_queue_info->tail < _queue_info->head)
        {
            size_t head_sz = _queue_info->capacity - _queue_info->head;
            if (head_sz < obj_sz)
            {
                uint8_t tmp_buf[obj_sz];
                size_t tail_sz = obj_sz - head_sz;
                memcpy(tmp_buf, &_data[_queue_info->head], head_sz);
                memcpy(&tmp_buf[0] + head_sz, _data, tail_sz);
                obj.deserialize(tmp_buf);
                _queue_info->head = tail_sz;
            }
        }
        else
        {
            obj.serialize(&_data[_queue_info->head]);
            _queue_info->head += obj_sz;
        }
        _queue_info->size -= obj_sz;
    }

    void lock_queue();
    void unlock_queue();
    void notify_not_full();
    void notify_not_empty();

    bool wait_fd_data_available(int fd, int timeout);
    void read_event_fd(int fd);

    ShmQueueInfo *_queue_info = nullptr;
    uint8_t *_data = nullptr;
    int _shm_id;
    int _epoll_fd;
    int _q_not_empty_fd;
    int _q_not_full_fd;
};

}

#endif /* __NNXCAM_VPROD_SHM_QUEUE_HPP_17758__ */
