#ifndef __NNXCAM_VPROD_FUTEX_HPP_14422__
#define __NNXCAM_VPROD_FUTEX_HPP_14422__

#include <atomic>

namespace aux {

class ShmFutex
{
public:
    ShmFutex(std::atomic<int>* atom);
    void lock();
    void unlock();
private:
    std::atomic<int>* _atom = nullptr;
};

class Futex
{
public:
    Futex();
    Futex(int value);
    void lock();
    void unlock();
private:
    std::atomic<int> _value;
};

}

#endif /* __NNXCAM_VPROD_FUTEX_HPP_14422__ */
