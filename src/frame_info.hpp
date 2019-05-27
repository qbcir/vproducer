#ifndef __NNXCAM_VPROD_FRAME_INFO_HPP_4134__
#define __NNXCAM_VPROD_FRAME_INFO_HPP_4134__

#include <python3.6m/Python.h>
#include <numpy/arrayobject.h>

namespace nnxcam {

class FrameInfo
{
public:
    typedef npy_int32 delta_t;
    typedef npy_uint8 occupancy_t;

    delta_t& dx(size_t i, size_t j);
    delta_t& dy(size_t i, size_t j);
    occupancy_t& occupancy(size_t i, size_t j);

    void interpolate_flow(FrameInfo& prev, FrameInfo& next);
    void fill_missing_vectors();
private:
    PyArrayObject* _dx = nullptr;
    PyArrayObject* _dy = nullptr;
    PyArrayObject* _occupancy = nullptr;

    size_t _width;
    size_t _height;
};

}

#endif /* __NNXCAM_VPROD_FRAME_INFO_HPP_4134__ */
