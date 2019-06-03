#include "futex.hpp"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

static inline void lock_atom(std::atomic<int>* atom)
{
    int c = 0;
    std::atomic_compare_exchange_strong_explicit(atom, &c, 1, std::memory_order_seq_cst, std::memory_order_seq_cst);
    if (c == 0)
    {
        return;
    }
    if (c == 1)
    {
        c = std::atomic_exchange_explicit(atom, 2, std::memory_order_seq_cst);
    }
    while (c != 0)
    {
        syscall(SYS_futex, (int*)atom, FUTEX_WAIT, 2, NULL, 0, 0);
        c = std::atomic_exchange_explicit(atom, 2, std::memory_order_seq_cst);
    }
}

static inline void unlock_atom(std::atomic<int>* atom)
{
    if (std::atomic_fetch_sub_explicit(atom, 1, std::memory_order_seq_cst) != 1)
    {
        atom->store(0);
        syscall(SYS_futex, (int*)atom, FUTEX_WAKE, 1, 0, 0, 0);
    }
}

namespace nnxcam {

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
