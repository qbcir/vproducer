#include "reader.hpp"
#include <coda/logger.h>
#include <coda/error.hpp>

extern "C"
{
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

namespace nnxcam {

Reader::Reader(const CameraConfig& camera_config, std::mutex &queue_lock, ShmQueue *shm_queue) :
    _queue_lock(queue_lock),
    _shm_queue(shm_queue),
    _camera_config(camera_config)
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
    if ((err = avformat_open_input(&_av_format_ctx, _camera_config.url.c_str(), NULL, NULL)) != 0)
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
    return got_picture_ptr > 0;
}

// FIXME
bool Reader::read_packets()
{
    thread_local AVPacket pkt, pktCopy;
    while(true)
    {
        if(_initialized)
        {
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
    auto next_pts = (_av_frame->pkt_dts != AV_NOPTS_VALUE ? _av_frame->pts : _pts + 1);
    _pts = _av_frame->pts != AV_NOPTS_VALUE ? _av_frame->pts : next_pts;
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
    std::lock_guard<std::mutex> lock(_queue_lock);
    size_t frame_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, _av_frame->width, _av_frame->height, 1);
    uint32_t frame_width = _av_frame->width;
    uint32_t frame_height = _av_frame->height;
    _shm_queue->put(_camera_config.id);
    _shm_queue->put(frame_width);
    _shm_queue->put(frame_height);
    _shm_queue->put(uint32_t(frame_bytes));
    _shm_queue->put([=](uint8_t* dst) {
        auto out_frame = av_frame_alloc();
        uint8_t out_frame_buf[frame_bytes];
        out_frame->width = _av_frame->width;
        out_frame->height = _av_frame->height;
        av_image_fill_arrays(out_frame->data, out_frame->linesize, out_frame_buf, AV_PIX_FMT_BGR24,
                             _av_frame->width, _av_frame->height, 1);
        auto scale_context = sws_getContext(_av_frame->width, _av_frame->height, static_cast<AVPixelFormat>(_av_frame->format),
                                            _av_frame->width, _av_frame->height, AV_PIX_FMT_BGR24,
                                            SWS_BICUBIC, NULL, NULL, NULL);
        sws_scale(scale_context, _av_frame->data, _av_frame->linesize,
                  0, _av_frame->height, out_frame->data, out_frame->linesize);
        sws_freeContext(scale_context);
        av_image_copy_to_buffer(dst, frame_bytes, out_frame->data, out_frame->linesize,
                                AV_PIX_FMT_BGR24, out_frame->width, out_frame->height, 1);
        av_frame_free(&out_frame);
    }, frame_bytes);

    FrameInfo curr_frame_info(_grid_size, _av_frame->width, _av_frame->height, _pts);
    for(size_t i = 0; i < _motion_vectors.size(); i++)
    {
        auto& mvec = _motion_vectors[i];
        curr_frame_info.add_motion_vector(mvec);
    }
    _shm_queue->put(curr_frame_info);
}

}
