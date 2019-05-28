#ifndef __NNXCAM_VPROD_SHM_MUTEX_HPP_14422__
#define __NNXCAM_VPROD_SHM_MUTEX_HPP_14422__

#include <atomic>

namespace nnxcam {

class Mutex
{
public:
    Mutex(std::atomic<int>* atom);
    void lock();
    void unlock();
private:
    std::atomic<int>* _atom = nullptr;
};

}

#endif /* __NNXCAM_VPROD_SHM_MUTEX_HPP_14422__ */
