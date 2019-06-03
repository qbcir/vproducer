#ifndef __NNXCAM_COMMON_SEMAPHORE_HPP_17640__
#define __NNXCAM_COMMON_SEMAPHORE_HPP_17640__

#include <atomic>

namespace nnxcam {

struct SemaphoreData
{
    std::atomic<int> atom;
    int futex;
};

class ShmSemaphore
{
public:
    ShmSemaphore(SemaphoreData* data);
    ShmSemaphore(std::atomic<int>* atom, int* futex);
    void wait();
    void notify();
private:
    std::atomic<int>* _atom;
    int* _futex;
};

}

#endif /* __NNXCAM_COMMON_SEMAPHORE_HPP_17640__ */
