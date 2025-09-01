#include <windows.h>

#include <iostream>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

#include "rtc_base/logging.h"

namespace ffmpeg_dyn {

namespace {
using av_packet_alloc_t = AVPacket* (*)();
using av_packet_free_t = void (*)(AVPacket** pkt);
using avcodec_find_encoder_by_name_t = const AVCodec* (*)(const char* name);
using avcodec_alloc_context3_t = AVCodecContext* (*)(const AVCodec* codec);
using av_opt_set_t = int (*)(void* obj, const char* name, const char* val, int search_flags);

using avcodec_open2_t = int (*)(AVCodecContext* ctx, const AVCodec* codec,
                                AVDictionary** options);
using av_frame_alloc_t = AVFrame* (*)(void);
using av_frame_get_buffer_t = int (*)(AVFrame* frame, int align);
using av_frame_free_t = void (*)(AVFrame **frame);
using avcodec_free_context_t = void (*)(AVCodecContext** ctx);
using av_frame_make_writable_t = int (*)(AVFrame* frame);
using avcodec_send_frame_t = int (*)(AVCodecContext* ctx, const AVFrame* frame);
using avcodec_receive_packet_t = int (*)(AVCodecContext* ctx, AVPacket* pkt);
using av_packet_unref_t = void (*)(AVPacket* pkt);
using av_opt_set_int_t = int (*)(void* obj, const char* name, int64_t val,
                                 int search_flags);
}  // namespace

inline av_packet_alloc_t av_packet_alloc_ptr = nullptr;
inline av_packet_free_t av_packet_free_ptr = nullptr;
inline avcodec_find_encoder_by_name_t avcodec_find_encoder_by_name_ptr =
    nullptr;
inline avcodec_alloc_context3_t avcodec_alloc_context3_ptr = nullptr;
inline av_opt_set_t av_opt_set_ptr = nullptr;
inline avcodec_open2_t avcodec_open2_ptr = nullptr;
inline av_frame_alloc_t av_frame_alloc_ptr = nullptr;
inline av_frame_get_buffer_t av_frame_get_buffer_ptr = nullptr;
inline av_frame_free_t av_frame_free_ptr = nullptr;
inline avcodec_free_context_t avcodec_free_context_ptr = nullptr;
inline av_frame_make_writable_t av_frame_make_writable_ptr = nullptr;
inline avcodec_send_frame_t avcodec_send_frame_ptr = nullptr;
inline avcodec_receive_packet_t avcodec_receive_packet_ptr = nullptr;
inline av_packet_unref_t av_packet_unref_ptr = nullptr;
inline av_opt_set_int_t av_opt_set_int_ptr = nullptr;

// av_packet_alloc av_packet_free avcodec_find_encoder_by_name
//    avcodec_alloc_context3 av_opt_set avcodec_open2 av_frame_alloc
//        av_frame_get_buffer av_frame_free avcodec_free_context
//            av_frame_make_writable avcodec_send_frame avcodec_receive_packet
//                av_packet_unref av_opt_set_int

bool init_avcodec();

}  // namespace ffmpeg_dyn