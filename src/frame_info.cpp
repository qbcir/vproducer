#include "frame_info.hpp"

namespace nnxcam {

FrameInfo::delta_t& FrameInfo::dx(size_t i, size_t j)
{
    auto ip = i*PyArray_STRIDES(_dx)[0];
    auto jp = j*PyArray_STRIDES(_dx)[1];
    return *(delta_t*)(PyArray_DATA(_dx) + ip + jp);
}

FrameInfo::delta_t& FrameInfo::dy(size_t i, size_t j)
{
    auto ip = i*PyArray_STRIDES(_dy)[0];
    auto jp = j*PyArray_STRIDES(_dy)[1];
    return *(delta_t*)(PyArray_DATA(_dy) + ip + jp);
}

FrameInfo::occupancy_t& FrameInfo::occupancy(size_t i, size_t j)
{
    auto ip = i*PyArray_STRIDES(_occupancy)[0];
    auto jp = j*PyArray_STRIDES(_occupancy)[1];
    return *(occupancy_t*)(PyArray_DATA(_occupancy) + ip + jp);
}

void FrameInfo::interpolate_flow(FrameInfo& prev, FrameInfo& next)
{
    for(int i = 0; i < _width; i++)
    {
        for(int j = 0; j < _height; j++)
        {
            dx(i, j) = (prev.dx(i, j) + next.dx(i, j)) / 2;
            dy(i, j) = (prev.dy(i, j) + next.dy(i, j)) / 2;
        }
    }
}

void FrameInfo::fill_missing_vectors()
{
    for(int k = 0; k < 2; k++)
    {
        for(size_t i = 1; i < _width - 1; i++)
        {
            for(size_t j = 1; j < _height - 1; j++)
            {
                if(occupancy(i, j) == 0)
                {
                    if(occupancy(i, j - 1) != 0 && occupancy(i, j + 1) != 0)
                    {
                        dx(i, j) = (dx(i, j -1) + dx(i, j + 1)) / 2;
                        dy(i, j) = (dy(i, j -1) + dy(i, j + 1)) / 2;
                        occupancy(i, j) = 2;
                    }
                    else if(occupancy(i - 1, j) != 0 && occupancy(i + 1, j) != 0)
                    {
                        dx(i, j) = (dx(i - 1, j) + dx(i + 1, j)) / 2;
                        dy(i, j) = (dy(i - 1, j) + dy(i + 1, j)) / 2;
                        occupancy(i, j) = 2;
                    }
                }
            }
        }
    }
}

}
