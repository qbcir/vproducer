#include "frame_info.hpp"
#include "common/serialize.hpp"

namespace nnxcam {

FrameInfo::FrameInfo(size_t grid_step_, size_t width_, size_t height_, int64_t pts_) :
    grid_step(grid_step_),
    pts(pts_)
{
    width = width_ / grid_step_;
    height = height_ / grid_step_;
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

size_t FrameInfo::serialize(uint8_t *dst) const
{
    auto p = dst;
    if (empty)
    {
        p += nnxcam::serialize<uint8_t>(p, 0);
        return p - dst;
    }
    p += nnxcam::serialize<uint8_t>(p, 1);
    p += nnxcam::serialize<int64_t>(p, pts);
    p += nnxcam::serialize<uint32_t>(p, grid_step);
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

ssize_t FrameInfo::deserialize(const uint8_t *p)
{
    return -1;
}

void FrameInfo::add_motion_vector(const AVMotionVector& mvec)
{
    int mvdx = mvec.dst_x - mvec.src_x;
    int mvdy = mvec.dst_y - mvec.src_y;

    int i = mvec.dst_x / grid_step;
    int j = mvec.dst_y / grid_step;
    size_t i_clipped = std::max(0, std::min(i, (int)width - 1));
    size_t j_clipped = std::max(0, std::min(j, (int)height - 1));

    empty = false;
    dx[i_clipped][j_clipped] = mvdx;
    dy[i_clipped][j_clipped] = mvdy;
    occupancy[i_clipped][j_clipped] = 1;
}

size_t FrameInfo::byte_size() const
{
    size_t sz = 0;
    if (empty)
    {
        sz += sizeof(uint8_t);
    }
    else
    {
        size_t grid_sz = width * height;
        sz += sizeof(uint8_t);
        sz += sizeof(grid_step);
        sz += sizeof(width);
        sz += sizeof(height);
        sz += sizeof(pts);
        sz += grid_sz * sizeof(delta_t);
        sz += grid_sz * sizeof(delta_t);
        sz += grid_sz * sizeof(occupancy_t);
    }
    return sz;
}

}
