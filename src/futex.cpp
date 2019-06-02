#include "futex.hpp"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

static inline int cmpxchg(std::atomic<int>* atom, int expected, int desired)
{
    int* ep = &expected;
    std::atomic_compare_exchange_strong(atom, ep, desired);
    return *ep;
}

static inline void lock_atom(std::atomic<int>* atom)
{
    int c = cmpxchg(atom, 0, 1);
    if (c != 0)
    {
        do
        {
            if (c == 2 || cmpxchg(atom, 1, 2) != 0)
            {
                syscall(SYS_futex, (int*)atom, FUTEX_WAIT, 2, 0, 0, 0);
            }
        }
        while ((c = cmpxchg(atom, 0, 2)) != 0);
    }
}

static inline void unlock_atom(std::atomic<int>* atom)
{
    if (atom->fetch_sub(1) != 1)
    {
        atom->store(0);
        syscall(SYS_futex, (int*)atom, FUTEX_WAKE, 1, 0, 0, 0);
    }
}

namespace aux {

ShmFutex::ShmFutex(std::atomic<int>* atom) :
    _atom(atom)
{}

void ShmFutex::lock()
{
    lock_atom(_atom);
}

void ShmFutex::unlock()
{
    unlock_atom(_atom);
}

Futex::Futex() :
    _value(0)
{
}

Futex::Futex(int value) :
    _value(value)
{
}

void Futex::lock()
{
    lock_atom(&_value);
}

void Futex::unlock()
{
    unlock_atom(&_value);
}

}
