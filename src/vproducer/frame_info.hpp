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

    size_t serialize(uint8_t *dst);
    bool deserialize(uint8_t *p);

    size_t byte_size() const;
private:
    std::vector<std::vector<delta_t>> _dx;
    std::vector<std::vector<delta_t>> _dy;
    std::vector<std::vector<occupancy_t>> _occupancy;

    uint32_t _width;
    uint32_t _height;
};

}

#endif /* __NNXCAM_VPROD_FRAME_INFO_HPP_4134__ */
