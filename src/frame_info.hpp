#ifndef __NNXCAM_VPROD_FRAME_INFO_HPP_4134__
#define __NNXCAM_VPROD_FRAME_INFO_HPP_4134__

#include <cstdint>
#include <vector>

namespace nnxcam {

class FrameInfo
{
public:
    typedef int32_t delta_t;
    typedef uint8_t occupancy_t;

    FrameInfo(size_t width, size_t height);

    void interpolate_flow(FrameInfo& prev, FrameInfo& next);
    void fill_missing_vectors();
private:
    std::vector<std::vector<delta_t>> _dx;
    std::vector<std::vector<delta_t>> _dy;
    std::vector<std::vector<occupancy_t>> _occupancy;

    size_t _width;
    size_t _height;
};

}

#endif /* __NNXCAM_VPROD_FRAME_INFO_HPP_4134__ */
