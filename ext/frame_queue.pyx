# distutils: language = c++
# distutils: sources = ../src/shm_queue.cpp ../src/shm_mutex.cpp ../src/utils.cpp

from libcpp cimport bool
from libcpp.string cimport string
from libc.stdint cimport uint8_t, uint32_t
from libc.stdio cimport printf

cdef extern from "shm_queue.hpp" namespace "nnxcam":
    cdef cppclass ShmQueue:
        bool get(uint8_t* dst, size_t data_sz)
        bool put(uint8_t *src, size_t data_sz)
        bool get(uint8_t *dst, size_t data_sz, int timeout)
        bool put(uint8_t* src, size_t data_sz, int timeout)
        bool get[T_](T_& obj)
        bool put[T_](const T_& obj)

#cdef class FrameQueue:
#    cdef queue[hit_t] _shm
#
#    def __init__(self, key_fname, bs, mbiq):
#        cdef char* _key_fname = key_fname
#        self._shm.connect(_key_fname, bs, mbiq)
#
#    def __dealloc__(self):
#        self._shm.flush()
#
#    def get(self):
#        cdef hit_t v
#        ok = self._shm.get(v)
#        return v if ok else None

