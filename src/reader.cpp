#include "reader.hpp"

namespace nnxcam {

Reader::Reader(const std::string &video_url) :
    _video_url(video_url)
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
    if ((err = avformat_open_input(&_av_format_ctx, _video_url.c_str(), NULL, NULL)) != 0)
    {
        //ffmpeg_print_error(err);
        //throw std::runtime_error("Couldn't open file. Possibly it doesn't exist.");
        return false;
    }
    if ((err = avformat_find_stream_info(_av_format_ctx, NULL)) < 0)
    {
        //ffmpeg_print_error(err);
        //throw std::runtime_error("Stream information not found.");
        return false;
    }
    for(size_t i = 0; i < _av_format_ctx->nb_streams; i++)
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
    return true;
}

int Reader::process_frame(AVPacket *pkt)
{
    av_frame_unref(_av_frame);

    int got_picture_ptr = 0;
    int ret = avcodec_decode_video2(_av_stream->codec, _av_frame, &got_picture_ptr, pkt);
    if (ret < 0)
    {
        return ret;
    }
    ret = FFMIN(ret, pkt->size); /* guard against bogus return values */
    pkt->data += ret;
    pkt->size -= ret;
    return got_picture_ptr;
}

// FIXME
int Reader::read_packets()
{
    static AVPacket pkt, pktCopy;
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
    if(read_packets() > 0)
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
    for(size_t frame_idx = 1; read_frame(); frame_idx++)
    {
        if(_pts <= _prev_pts && _prev_pts != -1)
        {
            //fprintf(stderr, "Skipping frame %d (frame with pts %d already processed).\n", int(frameIndex), int(pts));
            continue;
        }
        //output_vectors_std(frame_idx, pts, pictType, motionVectors);
        _prev_pts = _pts;
    }
}

}
