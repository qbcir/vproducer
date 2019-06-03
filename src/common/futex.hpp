#ifndef __NNXCAM_COMMON_FUTEX_HPP_14422__
#define __NNXCAM_COMMON_FUTEX_HPP_14422__

#include <atomic>

namespace nnxcam {

class ShmFutex
{
public:
    ShmFutex() {}
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

#endif /* __NNXCAM_COMMON_FUTEX_HPP_14422__ */
