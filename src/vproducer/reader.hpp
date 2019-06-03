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

class Reader
{
public:
    Reader(const std::string& video_url, std::mutex& queue_lock, std::shared_ptr<ShmQueue> shm_queue);
    ~Reader();

    bool init();
    bool run();
private:
    bool read_frame();
    int process_frame(AVPacket *pkt);
    int read_packets();
    void dump_frames();

    AVFrame* _av_frame = nullptr;
    AVFormatContext* _av_format_ctx = nullptr;
    AVStream* _av_stream = nullptr;
    int _video_stream_idx = -1;
    bool _initialized = false;

    const std::string _video_url;
    size_t _frame_width = 0;
    size_t _frame_height = 0;

    int64_t _pts = -1;
    int64_t _prev_pts = -1;
    char _pic_type;
    std::vector<AVMotionVector> _motion_vectors;

    std::mutex& _queue_lock;
    std::shared_ptr<ShmQueue> _shm_queue = nullptr;

    std::vector<FrameInfo> _frame_info_vec;

    size_t _frame_index = 0;

};

}

#endif /* __NNXCAM_VPROD_READER_HPP_20168__ */
