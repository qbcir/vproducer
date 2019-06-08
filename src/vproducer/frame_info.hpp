#ifndef __NNXCAM_VPROD_FRAME_INFO_HPP_4134__
#define __NNXCAM_VPROD_FRAME_INFO_HPP_4134__

#include <cstdint>
#include <vector>
#include <libavutil/motion_vector.h>
#include "common/serialize.hpp"

namespace nnxcam {

class FrameInfo : public Serializable
{
public:
    typedef int32_t delta_t;
    typedef uint8_t occupancy_t;

    FrameInfo(size_t grid_step, size_t width, size_t height, int64_t pts_);

    void interpolate_flow(FrameInfo& prev, FrameInfo& next);
    void fill_missing_vectors();

    size_t serialize(uint8_t *dst) const;
    ssize_t deserialize(const uint8_t *src);

    size_t byte_size() const;

    void add_motion_vector(const AVMotionVector& mvec);

    std::vector<std::vector<delta_t>> dx;
    std::vector<std::vector<delta_t>> dy;
    std::vector<std::vector<occupancy_t>> occupancy;

    uint32_t grid_step;
    uint32_t width;
    uint32_t height;

    bool empty = true;
    int64_t pts;
};

}

#endif /* __NNXCAM_VPROD_FRAME_INFO_HPP_4134__ */
