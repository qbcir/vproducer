# distutils: language = c++
# distutils: sources = ../src/common/shm_queue.cpp ../src/common/cond_var.cpp ../src/common/futex.cpp ../src/common/utils.cpp

cimport numpy as np
import numpy as np

from libcpp cimport bool
from libcpp.string cimport string
from libc.stdint cimport uint8_t, uint32_t, int32_t, int64_t
from libc.stdio cimport printf

cdef extern from "shm_queue.hpp" namespace "nnxcam":
    cdef cppclass ShmQueue:
        void connect(const char* key_fname, uint32_t q_size)
        bool get(uint8_t* dst, size_t data_sz)
        bool put(uint8_t *src, size_t data_sz)
        T_ get[T_](T_& obj)
        bool put[T_](T_& value)

cdef class FrameData:
    cdef uint32_t camera_id
    cdef const uint8_t [:,:,:] _frame
    cdef uint8_t has_motion_info
    cdef int64_t pts
    cdef uint32_t grid_step
    cdef const int32_t [:,:] _dx
    cdef const int32_t [:,:] _dy
    cdef const uint8_t [:,:] _occupancy
    #
    property frame:
        def __get__(self):
            return np.array(self._frame)

        def __set__(self, np.ndarray[np.uint8_t, ndim=3] frame):
            self._frame = frame
    #
    property dx:
        def __get__(self):
            return np.array(self._dx)

        def __set__(self, np.ndarray[np.int32_t, ndim=2] dx):
            self._dx = dx
    #
    property dy:
        def __get__(self):
            return np.array(self._dy)

        def __set__(self, np.ndarray[np.int32_t, ndim=2] dy):
            self._dy = dy
    #
    property occupancy:
        def __get__(self):
            return np.array(self._occupancy)

        def __set__(self, np.ndarray[np.uint8_t, ndim=2] occupancy):
            self._occupancy = occupancy

cdef class FrameQueue:
    cdef ShmQueue _shm_queue

    def __init__(self, key_fname, queue_size):
        cdef char* _key_fname = key_fname
        self._shm_queue.connect(_key_fname, queue_size)

    def get(self):
        frame_data = FrameData()
        frame_data.camera_id = self._shm_queue.get[uint32_t]()
        frame_width = self._shm_queue.get[uint32_t]()
        frame_height = self._shm_queue.get[uint32_t]()
        frame_sz = self._shm_queue.get[uint32_t]()
        #
        cdef np.ndarray[np.uint8_t, ndim=3] frame
        frame = np.empty((frame_height, frame_width, 3), dtype=np.uint8)
        self._shm_queue.get(<uint8_t*>frame.data, frame_sz)
        frame_data.frame = frame
        #
        #return frame_data
        #
        frame_data.has_motion_info = self._shm_queue.get[uint8_t]()
        if frame_data.has_motion_info == 0:
            return frame_data
        #
        frame_data.pts = self._shm_queue.get[int64_t]()
        frame_data.grid_step = self._shm_queue.get[uint32_t]()
        #
        width = self._shm_queue.get[uint32_t]()
        height = self._shm_queue.get[uint32_t]()
        data_sz = width * height
        # dx
        cdef np.ndarray[np.int32_t, ndim=2] dx
        dx = np.empty((width, height), dtype=np.int32)
        self._shm_queue.get(<uint8_t*>dx.data, data_sz*4)
        frame_data.dx = dx
        # dy
        cdef np.ndarray[np.int32_t, ndim=2] dy
        dy = np.empty((width, height), dtype=np.int32)
        self._shm_queue.get(<uint8_t*>dy.data, data_sz*4)
        frame_data.dy = dy
        # occupancy
        cdef np.ndarray[np.uint8_t, ndim=2] occupancy
        occupancy = np.empty((width, height), dtype=np.uint8)
        self._shm_queue.get(<uint8_t*>occupancy.data, data_sz)
        frame_data.occupancy = occupancy
        #
        return frame_data

