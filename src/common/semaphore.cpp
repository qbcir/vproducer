#include "semaphore.hpp"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <sched.h>
#include <unistd.h>

static void wait_atom(std::atomic<int>* atom, int* futex) {
    int value = std::atomic_fetch_sub_explicit(atom, 1, std::memory_order_relaxed);
    if (value < 0)
    {
        syscall(SYS_futex, futex, FUTEX_WAIT, *futex, NULL, 0, 0);
    }
}

static void notify_atom(std::atomic<int>* atom, int* futex)
{
    int value = std::atomic_fetch_add_explicit(atom, 1, std::memory_order_relaxed);
    if (value <= 0)
    {
        while (syscall(SYS_futex, futex, FUTEX_WAKE, 1, NULL, 0, 0) < 1)
        {
            sched_yield();
        }
    }
}

namespace nnxcam {

ShmSemaphore::ShmSemaphore(SemaphoreData* data) :
    _atom(&data->atom),
    _futex(&data->futex)
{
}

ShmSemaphore::ShmSemaphore(std::atomic<int>* atom, int* futex) :
    _atom(atom),
    _futex(futex)
{
}

void ShmSemaphore::wait()
{
    wait_atom(_atom, _futex);
}

void ShmSemaphore::notify()
{
    notify_atom(_atom, _futex);
}

}
