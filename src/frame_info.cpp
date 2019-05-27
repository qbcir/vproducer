#include "frame_info.hpp"

namespace nnxcam {

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

}
