#include "shm_queue.hpp"
#include "utils.hpp"
//
#include <chrono>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <coda/error.hpp>

namespace nnxcam {

ShmQueue::ShmQueue() :
    _queue_lock(nullptr)
{   
}

void ShmQueue::connect(const std::string& key, uint32_t queue_size)
{
    connect_sem(key.c_str());
    lock_queue();
    connect_shm(key.c_str(), queue_size);
    unlock_queue();
}

void ShmQueue::lock_queue()
{
    struct sembuf sem;
    sem.sem_num = 0;
    sem.sem_op = -1;
    sem.sem_flg = SEM_UNDO;
    semop(_sem_id, &sem, 1);
}

void ShmQueue::unlock_queue()
{
    struct sembuf sem;
    sem.sem_num = 0;
    sem.sem_op = 1;
    sem.sem_flg = SEM_UNDO;
    semop(_sem_id, &sem, 1);
}

void ShmQueue::connect_sem(const char *key_fname)
{
    if (-1 != _sem_id)
    {
        throw coda_error("ShmQueue::connect_sem: sem already connected");
    }
    key_t key;
    if ((key_t) -1 == (key = ftok(key_fname, 1)))
    {
        throw coda_error("ShmQueue::connect_sem: error creating key from file '%s': %s", key_fname, coda_strerror(errno));
    }
    if (-1 == (_sem_id = semget(key, 2, 0)))
    {
        if (-1 == (_sem_id = semget(key, 2, IPC_CREAT | 0666)))
        {
            throw coda_error("ShmQueue::connect_sem: error getting sem: %s", coda_strerror(errno));
        }

        if (-1 == semctl(_sem_id, 0, SETVAL, (1)))
        {
            throw coda_error("ShmQueue::connect_sem: error setting sem: %s", coda_strerror(errno));
        }

        if (-1 == semctl(_sem_id, 1, SETVAL, (0)))
        {
            throw coda_error("ShmQueue::connect_sem: error setting sem: %s", coda_strerror(errno));
        }
    }
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
        if (-1 == (_shm_id = shmget(key, q_size + sizeof(_queue_info), IPC_CREAT | 0666)))
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
        _queue_info->capacity = q_size;
    }
    _queue_lock = ShmFutex(&_queue_info->atom);
    _not_empty_cond = ShmCondVar(&_queue_info->not_empty);
    _not_full_cond = ShmCondVar(&_queue_info->not_full);
    ptr += sizeof(ShmQueueInfo);
    _data = ptr;
}

bool ShmQueue::get(uint8_t *dst, size_t data_sz)
{
    return get(dst, data_sz, -1);
}

bool ShmQueue::put(uint8_t* src, size_t data_sz)
{
    return put(src, data_sz, -1);
}

bool ShmQueue::get(uint8_t* dst, size_t data_sz, int timeout)
{
    return get([=](uint8_t* _src) {
        memcpy(dst, _src, data_sz);
    }, data_sz, timeout);
}

bool ShmQueue::put(uint8_t *src, size_t data_sz, int timeout)
{
    return put([=](uint8_t* _dst) {
        memcpy(_dst, src, data_sz);
    }, data_sz, timeout);
}

}
