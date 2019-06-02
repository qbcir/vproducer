#include "frame_info.hpp"
#include "serialize.hpp"

namespace aux {

FrameInfo::FrameInfo(size_t width, size_t height) :
    _width(width),
    _height(height)
{
    _dx.resize(width);
    for (auto& v : _dx)
    {
        v.resize(height);
    }
    _dy.resize(width);
    for (auto& v : _dy)
    {
        v.resize(height);
    }
    _occupancy.resize(width);
    for (auto& v : _occupancy)
    {
        v.resize(height);
    }
}

void FrameInfo::interpolate_flow(FrameInfo& prev, FrameInfo& next)
{
    for(size_t i = 0; i < _width; i++)
    {
        for(size_t j = 0; j < _height; j++)
        {
            _dx[i][j] = (prev._dx[i][j] + next._dx[i][j]) / 2;
            _dy[i][j] = (prev._dy[i][j] + next._dy[i][j]) / 2;
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
                if(!_occupancy[i][j])
                {
                    if(_occupancy[i][j - 1] && _occupancy[i][j + 1])
                    {
                        _dx[i][j] = (_dx[i][j -1] + _dx[i][j + 1]) / 2;
                        _dy[i][j] = (_dy[i][j -1] + _dy[i][j + 1]) / 2;
                        _occupancy[i][j] = 2;
                    }
                    else if(_occupancy[i - 1][j] && _occupancy[i + 1][j])
                    {
                        _dx[i][j] = (_dx[i - 1][j] + _dx[i + 1][j]) / 2;
                        _dy[i][j] = (_dy[i - 1][j] + _dy[i + 1][j]) / 2;
                        _occupancy[i][j] = 2;
                    }
                }
            }
        }
    }
}

size_t FrameInfo::serialize(uint8_t *dst)
{
    auto p = dst;
    p += aux::serialize<uint32_t>(p, _width);
    p += aux::serialize<uint32_t>(p, _height);
    for (size_t i = 0; i < _width; i++)
    {
        for (size_t j = 0; j < _height; j++)
        {
            p += aux::serialize<delta_t>(p, _dx[i][j]);
        }
    }
    for (size_t i = 0; i < _width; i++)
    {
        for (size_t j = 0; j < _height; j++)
        {
            p += aux::serialize<delta_t>(p, _dy[i][j]);
        }
    }
    for (size_t i = 0; i < _width; i++)
    {
        for (size_t j = 0; j < _height; j++)
        {
            p += aux::serialize<occupancy_t>(p, _occupancy[i][j]);
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
    size_t grid_sz = _width * _height;
    sz += grid_sz * sizeof(delta_t);
    sz += grid_sz * sizeof(delta_t);
    sz += grid_sz * sizeof(occupancy_t);
    return sz;
}

}
