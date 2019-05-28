#include "shm_mutex.hpp"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

int cmpxchg(std::atomic<int>* atom, int expected, int desired)
{
    int* ep = &expected;
    std::atomic_compare_exchange_strong(atom, ep, desired);
    return *ep;
}

namespace nnxcam {

Mutex::Mutex(std::atomic<int>* atom) :
    _atom(atom)
{}

void Mutex::lock()
{
    int c = cmpxchg(_atom, 0, 1);
    if (c != 0)
    {
        do
        {
            if (c == 2 || cmpxchg(_atom, 1, 2) != 0)
            {
                syscall(SYS_futex, (int*)_atom, FUTEX_WAIT, 2, 0, 0, 0);
            }
        }
        while ((c = cmpxchg(_atom, 0, 2)) != 0);
    }
}

void Mutex::unlock()
{
    if (_atom->fetch_sub(1) != 1)
    {
        _atom->store(0);
        syscall(SYS_futex, (int*)_atom, FUTEX_WAKE, 1, 0, 0, 0);
    }
}

}
