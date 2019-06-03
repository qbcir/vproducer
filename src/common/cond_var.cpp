#include "cond_var.hpp"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace nnxcam {

static void wait_cond(std::atomic_int* atom, std::atomic_uint* previous, std::unique_lock<ShmFutex>& futex)
{
    int value = std::atomic_load_explicit(atom, std::memory_order_relaxed);
    std::atomic_store_explicit(previous, (unsigned int)value, std::memory_order_relaxed);
    futex.unlock();
    syscall(SYS_futex, (int*)atom, FUTEX_WAIT, value, NULL, 0, 0);
    futex.lock();
}

static void notify_cond(std::atomic_int* atom, std::atomic_uint* previous)
{
    unsigned value = 1u + std::atomic_load_explicit(previous, std::memory_order_relaxed);
    std::atomic_store_explicit(atom, int(value), std::memory_order_relaxed);
    syscall(SYS_futex, (int*)atom, FUTEX_WAKE, 1, NULL, 0, 0);
}

ShmCondVar::ShmCondVar(CondVarData* data) :
    _value(&data->value),
    _previous(&data->previous)
{
}

void ShmCondVar::wait(std::unique_lock<ShmFutex>& futex)
{
    wait_cond(_value, _previous, futex);
}

void ShmCondVar::notify()
{
    notify_cond(_value, _previous);
}

}
