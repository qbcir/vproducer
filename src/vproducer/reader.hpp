#ifndef __NNXCAM_VPROD_READER_HPP_20168__
#define __NNXCAM_VPROD_READER_HPP_20168__

#include <string>
#include <mutex>
#include <vector>
#include <memory>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/motion_vector.h>
}

#include "common/shm_queue.hpp"
#include "frame_info.hpp"

namespace nnxcam {

struct CameraConfig
{
    uint32_t id;
    std::string url;
};

class Reader
{
public:
    Reader(const CameraConfig& camera_config, std::mutex& queue_lock, ShmQueue* shm_queue);
    ~Reader();

    bool init();
    bool run();
private:
    bool read_frame();
    bool process_frame(AVPacket *pkt);
    bool read_packets();
    void dump_frames();

    AVFrame* _av_frame = nullptr;
    AVFormatContext* _av_format_ctx = nullptr;
    AVStream* _av_stream = nullptr;
    int _video_stream_idx = -1;
    bool _initialized = false;

    size_t _frame_width = 0;
    size_t _frame_height = 0;

    int64_t _pts = -1;
    int64_t _prev_pts = -1;
    char _pic_type;
    std::vector<AVMotionVector> _motion_vectors;

    std::mutex& _queue_lock;
    ShmQueue* _shm_queue = nullptr;

    size_t _grid_size = 8;
    size_t _frame_index = 0;

    const CameraConfig _camera_config;
};

}

#endif /* __NNXCAM_VPROD_READER_HPP_20168__ */
