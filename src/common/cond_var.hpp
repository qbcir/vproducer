#ifndef __NNXCAM_COMMON_COND_VAR_HPP_25164__
#define __NNXCAM_COMMON_COND_VAR_HPP_25164__

#include "futex.hpp"
#include <mutex>

namespace nnxcam {

struct CondVarData
{
    std::atomic_int value;
    std::atomic_uint previous;
};

class ShmCondVar
{
public:
    ShmCondVar() {}
    ShmCondVar(CondVarData* data);
    void wait(std::unique_lock<ShmFutex> &futex);
    void notify();

private:
    std::atomic_int* _value = nullptr;
    std::atomic_uint* _previous = nullptr;
};

}

#endif /* __NNXCAM_COMMON_COND_VAR_HPP_25164__ */
