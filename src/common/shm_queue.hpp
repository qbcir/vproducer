#ifndef __NNXCAM_COMMON_SHM_QUEUE_HPP_17758__
#define __NNXCAM_COMMON_SHM_QUEUE_HPP_17758__

#include <atomic>
#include <cstddef>
#include <cstring>
#include <vector>
#include <string>
#include <mutex>
#include <type_traits>
#include <sys/time.h>
#include <time.h>

#include "futex.hpp"
#include "cond_var.hpp"
#include "serialize.hpp"

#include <inttypes.h>
#include <coda/logger.h>

namespace nnxcam {

struct ShmQueueInfo
{
    std::atomic<int32_t> atom;
    CondVarData not_full;
    CondVarData not_empty;
    uint32_t size;
    uint32_t capacity;
    uint32_t head;
    uint32_t tail;

    void reset(uint32_t _capacity)
    {
        atom = 0;
        size = 0;
        capacity = _capacity;
        head = 0;
        tail = 0;
    }
};

class ShmQueue
{
public:
    typedef uint32_t queue_size_t;

    ShmQueue();
    void connect(const std::string& key, uint32_t queue_size);
    //
    template<typename F, NNXCAM_ENABLE_IF_IS_SERIALIZE_FUNC(F)>
    inline bool get(F func, size_t data_sz)
    {
        return get<F>(func, data_sz, -1);
    }
    template<typename F, NNXCAM_ENABLE_IF_IS_SERIALIZE_FUNC(F)>
    inline bool put(F func, size_t data_sz)
    {
        return put<F>(func, data_sz, -1);
    }
    //
    template<typename F, NNXCAM_ENABLE_IF_IS_SERIALIZE_FUNC(F)>
    bool get(F func, size_t data_sz, int timeout)
    {
        std::unique_lock<ShmFutex> lock(_queue_lock);
        while (_queue_info->size < data_sz)
        {
            _not_empty_cond.wait(lock);
        }
        _deserialize(func, data_sz);
        _not_full_cond.notify();
        return true;
    }
    template<typename F, NNXCAM_ENABLE_IF_IS_SERIALIZE_FUNC(F)>
    bool put(F func, size_t data_sz, int timeout)
    {
        std::unique_lock<ShmFutex> lock(_queue_lock);
        while (_queue_info->size + data_sz >= _queue_info->capacity)
        {
            _not_full_cond.wait(lock);
        }
        _serialize(func, data_sz);
        _not_empty_cond.notify();
        return true;
    }
    //
    template<typename T, NNXCAM_ENABLE_IF_IS_SERIALIZABLE(T)>
    inline bool get(T& obj)
    {
        return get<T>(obj, -1);
    }
    template<typename T, NNXCAM_ENABLE_IF_IS_SERIALIZABLE(T)>
    inline bool put(const T& obj)
    {
        return put<T>(obj, -1);
    }
    //
    template<typename T, NNXCAM_ENABLE_IF_IS_SERIALIZABLE(T)>
    bool get(T& obj, int timeout)
    {
        return get([=] (uint8_t* src) {
            obj.deserialize(src);
        }, obj.byte_size(), timeout);
    }
    template<typename T, NNXCAM_ENABLE_IF_IS_SERIALIZABLE(T)>
    bool put(const T& obj, int timeout)
    {
        return put([=] (uint8_t* dst) {
            obj.serialize(dst);
        }, obj.byte_size(), timeout);
    }
    //
    template<typename T, NNXCAM_ENABLE_IF_IS_INT(T)>
    bool put(T value)
    {
        return put((uint8_t*)&value, sizeof(T));
    }
    template<typename T, NNXCAM_ENABLE_IF_IS_INT(T)>
    T get()
    {
        T value;
        get((uint8_t*)&value, sizeof(T));
        return value;
    }
    //
    bool get(uint8_t* dst, size_t data_sz);
    bool put(uint8_t *src, size_t data_sz);
    //
    bool get(uint8_t *dst, size_t data_sz, int timeout);
    bool put(uint8_t* src, size_t data_sz, int timeout);
    //
private:
    template<typename F>
    void _serialize(F func, size_t data_sz)
    {
        if (_queue_info->tail >= _queue_info->head)
        {
            size_t tail_sz = _queue_info->capacity - _queue_info->tail;
            if (tail_sz <= data_sz)
            {
                uint8_t tmp_buf[data_sz];
                func(tmp_buf);
                size_t head_sz = data_sz - tail_sz;
                memcpy(&_data[_queue_info->tail], tmp_buf, tail_sz);
                memcpy(_data, tmp_buf + tail_sz, head_sz);
                _queue_info->tail = head_sz;
                _queue_info->size += data_sz;
                return;
            }
        }
        func(&_data[_queue_info->tail]);
        _queue_info->tail += data_sz;
        _queue_info->size += data_sz;
    }

    template<typename F>
    void _deserialize(F func, size_t data_sz)
    {
        if (_queue_info->tail < _queue_info->head)
        {
            size_t head_sz = _queue_info->capacity - _queue_info->head;
            if (head_sz < data_sz)
            {
                uint8_t tmp_buf[data_sz];
                size_t tail_sz = data_sz - head_sz;
                memcpy(tmp_buf, &_data[_queue_info->head], head_sz);
                memcpy(&tmp_buf[0] + head_sz, _data, tail_sz);
                func(tmp_buf);
                _queue_info->head = tail_sz;
                _queue_info->size -= data_sz;
                return;
            }
        }
        func(&_data[_queue_info->head]);
        _queue_info->head += data_sz;
        _queue_info->size -= data_sz;
    }

    void lock_queue();
    void unlock_queue();
    void connect_sem(const char *key_fname);
    void connect_shm(const char* key_fname, size_t q_size);

    ShmQueueInfo *_queue_info = nullptr;
    ShmFutex _queue_lock;
    ShmCondVar _not_empty_cond;
    ShmCondVar _not_full_cond;
    uint8_t *_data = nullptr;
    int _shm_id = -1;
    int _sem_id = -1;
};

}

#endif /* __NNXCAM_COMMON_SHM_QUEUE_HPP_17758__ */
