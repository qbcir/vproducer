#include "frame_info.hpp"
#include "common/serialize.hpp"

namespace nnxcam {

FrameInfo::FrameInfo(size_t width, size_t height) :
    width(width),
    height(height)
{
    dx.resize(width);
    for (auto& v : dx)
    {
        v.resize(height);
    }
    dy.resize(width);
    for (auto& v : dy)
    {
        v.resize(height);
    }
    occupancy.resize(width);
    for (auto& v : occupancy)
    {
        v.resize(height);
    }
}

void FrameInfo::interpolate_flow(FrameInfo& prev, FrameInfo& next)
{
    for(size_t i = 0; i < width; i++)
    {
        for(size_t j = 0; j < height; j++)
        {
            dx[i][j] = (prev.dx[i][j] + next.dx[i][j]) / 2;
            dy[i][j] = (prev.dy[i][j] + next.dy[i][j]) / 2;
        }
    }
}

void FrameInfo::fill_missing_vectors()
{
    for(int k = 0; k < 2; k++)
    {
        for(size_t i = 1; i < width - 1; i++)
        {
            for(size_t j = 1; j < height - 1; j++)
            {
                if(!occupancy[i][j])
                {
                    if(occupancy[i][j - 1] && occupancy[i][j + 1])
                    {
                        dx[i][j] = (dx[i][j -1] + dx[i][j + 1]) / 2;
                        dy[i][j] = (dy[i][j -1] + dy[i][j + 1]) / 2;
                        occupancy[i][j] = 2;
                    }
                    else if(occupancy[i - 1][j] && occupancy[i + 1][j])
                    {
                        dx[i][j] = (dx[i - 1][j] + dx[i + 1][j]) / 2;
                        dy[i][j] = (dy[i - 1][j] + dy[i + 1][j]) / 2;
                        occupancy[i][j] = 2;
                    }
                }
            }
        }
    }
}

size_t FrameInfo::serialize(uint8_t *dst)
{
    auto p = dst;
    p += nnxcam::serialize<uint32_t>(p, width);
    p += nnxcam::serialize<uint32_t>(p, height);
    for (size_t i = 0; i < width; i++)
    {
        for (size_t j = 0; j < height; j++)
        {
            p += nnxcam::serialize<delta_t>(p, dx[i][j]);
        }
    }
    for (size_t i = 0; i < width; i++)
    {
        for (size_t j = 0; j < height; j++)
        {
            p += nnxcam::serialize<delta_t>(p, dy[i][j]);
        }
    }
    for (size_t i = 0; i < width; i++)
    {
        for (size_t j = 0; j < height; j++)
        {
            p += nnxcam::serialize<occupancy_t>(p, occupancy[i][j]);
        }
    }
    return p - dst;
}

bool FrameInfo::deserialize(uint8_t *p)
{

}

size_t FrameInfo::byte_size() const
{
    size_t sz = 0;
    size_t grid_sz = width * height;
    sz += grid_sz * sizeof(delta_t);
    sz += grid_sz * sizeof(delta_t);
    sz += grid_sz * sizeof(occupancy_t);
    return sz;
}

}
