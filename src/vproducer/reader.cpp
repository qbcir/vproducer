#include "reader.hpp"
#include <coda/logger.h>
#include <coda/error.hpp>

extern "C"
{
#include <libavutil/opt.h>
}

namespace nnxcam {

Reader::Reader(const std::string &video_url, std::mutex &queue_lock, std::shared_ptr<ShmQueue> shm_queue) :
    _video_url(video_url),
    _queue_lock(queue_lock),
    _shm_queue(shm_queue)
{
    _av_frame = av_frame_alloc();
    _av_format_ctx = avformat_alloc_context();
}

Reader::~Reader()
{
    av_frame_free(&_av_frame);
    avformat_free_context(_av_format_ctx);
}

bool Reader::init()
{
    int err = 0;
    AVDictionary *options = nullptr;
    if((err = av_dict_set(&options, "rtsp_transport", "tcp", 0)) < 0)
    {
        throw coda_error("Can't set 'rtsp_transport' option.");
        return false;
    }
    if ((err = avformat_open_input(&_av_format_ctx, _video_url.c_str(), NULL, NULL)) != 0)
    {
        //ffmpeg_print_error(err);
        throw coda_error("Couldn't open file. Possibly it doesn't exist.");
        return false;
    }
    if ((err = avformat_find_stream_info(_av_format_ctx, NULL)) < 0)
    {
        //ffmpeg_print_error(err);
        throw coda_error("Stream information not found.");
        return false;
    }

    size_t i = 0;
    for(; i < _av_format_ctx->nb_streams; i++)
    {
        auto enc = _av_format_ctx->streams[i]->codec;
        if( AVMEDIA_TYPE_VIDEO == enc->codec_type && _video_stream_idx < 0)
        {
            auto codec = avcodec_find_decoder(enc->codec_id);
            if (!codec)
            {
                return false;
            }
            AVDictionary* opts = nullptr;
            av_dict_set(&opts, "flags2", "+export_mvs", 0);
            if (avcodec_open2(enc, codec, &opts) < 0)
            {
                return false;
            }
            _video_stream_idx = i;
            _av_stream = _av_format_ctx->streams[i];
            _frame_width = enc->width;
            _frame_height = enc->height;
            break;
        }
    }
    return i < _av_format_ctx->nb_streams;
}

bool Reader::process_frame(AVPacket *pkt)
{
    av_frame_unref(_av_frame);

    int got_picture_ptr = 0;
    int ret = avcodec_decode_video2(_av_stream->codec, _av_frame, &got_picture_ptr, pkt);
    if (ret < 0)
    {
        return false;
    }
    ret = FFMIN(ret, pkt->size); /* guard against bogus return values */
    pkt->data += ret;
    pkt->size -= ret;
    log_error("got_picture_ptr=%d", got_picture_ptr);
    return got_picture_ptr > 0;
}

// FIXME
bool Reader::read_packets()
{
    log_error("read_packets");
    thread_local AVPacket pkt, pktCopy;
    while(true)
    {
        if(_initialized)
        {
            log_error("initialized");
            if(process_frame(&pktCopy))
            {
                return true;
            }
            else
            {
                av_packet_unref(&pkt);
                _initialized = false;
            }
        }
        int ret = av_read_frame(_av_format_ctx, &pkt);
        log_error("ret=%d", ret);
        if(ret != 0)
        {
            break;
        }
        _initialized = true;
        pktCopy = pkt;
        if(pkt.stream_index != _video_stream_idx)
        {
            av_packet_unref(&pkt);
            _initialized = false;
            continue;
        }
    }
    log_error("process_frame");
    return process_frame(&pkt);
}

bool Reader::read_frame()
{
    if(!read_packets())
    {
        return false;
    }
    _pic_type = av_get_picture_type_char(_av_frame->pict_type);
    // fragile, consult fresh f_select.c and ffprobe.c when updating ffmpeg
    auto next_pts = (_av_frame->pkt_dts != AV_NOPTS_VALUE ? _av_frame->pkt_dts : _pts + 1);
    _pts = _av_frame->pkt_pts != AV_NOPTS_VALUE ? _av_frame->pkt_pts : next_pts;
    // reading motion vectors, see ff_print_debug_info2 in ffmpeg's libavcodec/mpegvideo.c for reference and a fresh doc/examples/extract_mvs.c
    auto motion_vectors_data = av_frame_get_side_data(_av_frame, AV_FRAME_DATA_MOTION_VECTORS);
    if (motion_vectors_data)
    {
        AVMotionVector* mvs = (AVMotionVector*)motion_vectors_data->data;
        auto mvcount = motion_vectors_data->size / sizeof(AVMotionVector);
        _motion_vectors = std::vector<AVMotionVector>(mvs, mvs + mvcount);
    }
    else
    {
        std::vector<AVMotionVector>().swap(_motion_vectors);
    }
    return true;
}

bool Reader::run()
{
    for(_frame_index = 1; read_frame(); _frame_index++)
    {
        if(_pts <= _prev_pts && _prev_pts != -1)
        {
            //fprintf(stderr, "Skipping frame %d (frame with pts %d already processed).\n", int(frameIndex), int(pts));
            continue;
        }
        dump_frames();
        _prev_pts = _pts;
    }
}

void Reader::dump_frames()
{
    size_t frame_bytes = avpicture_get_size(AV_PIX_FMT_RGB32, _frame_width, _frame_height);
    uint8_t frame_buf [frame_bytes];
    avpicture_layout((AVPicture*)_av_frame, AV_PIX_FMT_RGB32, _frame_width, _frame_height, frame_buf, frame_bytes);

    /*
    size_t grid_step = 8;

    auto grid_width = _frame_width / grid_step;
    auto grid_height = _frame_height / grid_step;

    if(!_frame_info_vec.empty() && _pts != _frame_info_vec.back().pts + 1)
    {
        for(int64_t dummy_pts = _frame_info_vec.back().pts + 1; dummy_pts < _pts; dummy_pts++)
        {
            FrameInfo dummy(grid_width, grid_height);
            dummy.FrameIndex = -1;
            dummy.pts = dummy_pts;
            dummy.Origin = "dummy";
            dummy.PictType = '?';
            dummy.GridStep = gridStep;
            prev.push_back(dummy);
        }
    }

    FrameInfo curr_frame_info(grid_width, grid_height);

    cur.FrameIndex = frameIndex;
    cur.Pts = pts;
    cur.Origin = "video";
    cur.PictType = pictType;
    cur.GridStep = gridStep;
    cur.Shape = shape;


    for(int i = 0; i < _motion_vectors.size(); i++)
    {
        auto& mvec = _motion_vectors[i];
        int mvdx = mvec.dst_x - mvec.src_x;
        int mvdy = mvec.dst_y - mvec.src_y;

        size_t i_clipped = std::max(size_t(0), std::min(mvec.dst_x / grid_step, curr_frame_info.width - 1));
        size_t j_clipped = std::max(size_t(0), std::min(mvec.dst_y / grid_step, curr_frame_info.height - 1));

        curr_frame_info.empty = false;
        curr_frame_info.dx[i_clipped][j_clipped] = mvdx;
        curr_frame_info.dy[i_clipped][j_clipped] = mvdy;
        curr_frame_info.occupancy[i_clipped][j_clipped] = true;
    }
    if(grid_step == 8)
    {
        curr_frame_info.fill_missing_vectors();
    }
    if(_frame_index == -1)
    {
        for(int i = 0; i < prev.size(); i++)
            prev[i].PrintIfNotPrinted();
    }
    else if(!_motion_vectors.empty())
    {
        if(prev.size() == 2 && prev.front().Empty == false)
        {
            prev.back().InterpolateFlow(prev.front(), cur);
            prev.back().PrintIfNotPrinted();
        }
        else
        {
            for(int i = 0; i < prev.size(); i++)
                prev[i].PrintIfNotPrinted();
        }
        prev.clear();
        cur.PrintIfNotPrinted();
    }

    _frame_info_vec.emplace_back(curr_frame_info);
    */
}

}
