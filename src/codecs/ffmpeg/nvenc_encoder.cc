#include "src/codecs/ffmpeg/nvenc_encoder.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>

#include "modules/video_coding/include/video_codec_interface.h"
#include "modules/video_coding/include/video_error_codes.h"
#include "rtc_base/logging.h"
#include "src/codecs/ffmpeg/dyn.h"

namespace libwebrtc {

using namespace ffmpeg_dyn;

namespace {
static const char* kNvencName = "h264_nvenc";

struct ScopedPacket {
  AVPacket* pkt = av_packet_alloc_ptr();
  ~ScopedPacket() {
    if (pkt) av_packet_free_ptr(&pkt);
  }
};
}  // namespace

struct FFmpegState {
  const AVCodec* codec = nullptr;
  AVCodecContext* ctx = nullptr;
  AVFrame* frame = nullptr;
};

NVH264Encoder::NVH264Encoder() {}
NVH264Encoder::~NVH264Encoder() { Release(); }

int32_t NVH264Encoder::InitEncode(const webrtc::VideoCodec* codec_settings,
                                  const Settings& settings) {
  RTC_LOG(LS_INFO) << "NVENC InitEncode";
  if (!codec_settings) return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  std::lock_guard<std::mutex> lock(mutex_);

  codec_ = *codec_settings;
  width_ = codec_.width;
  height_ = codec_.height;
  fps_ = std::max<uint32_t>(1, codec_.maxFramerate);

  ff_ = std::make_unique<FFmpegState>();
  if (OpenCodec(codec_) != 0) {
    CloseCodec();
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  initialized_ = true;
  return WEBRTC_VIDEO_CODEC_OK;
}

int NVH264Encoder::OpenCodec(const webrtc::VideoCodec& vcodec) {
  if (!init_avcodec()) {
    RTC_LOG(LS_ERROR) << "FFmpeg avcodec not loaded";
    return -1;
  }

  ff_->codec = avcodec_find_encoder_by_name_ptr(kNvencName);
  if (!ff_->codec) {
    RTC_LOG(LS_ERROR) << "FFmpeg encoder not found: " << kNvencName;
    return -1;
  }

  ff_->ctx = avcodec_alloc_context3_ptr(ff_->codec);
  if (!ff_->ctx) return -1;

  ff_->ctx->width = width_;
  ff_->ctx->height = height_;
  ff_->ctx->time_base = AVRational{1, static_cast<int>(fps_)};
  ff_->ctx->framerate = AVRational{static_cast<int>(fps_), 1};
  ff_->ctx->pix_fmt = AV_PIX_FMT_YUV420P;  // Input from WebRTC (I420)

  // Bitrate
  int64_t bps = vcodec.maxBitrate ? vcodec.maxBitrate * 1000LL
                                  : vcodec.startBitrate * 1000LL;
  if (bps <= 0) bps = 2'000'000;
  ff_->ctx->bit_rate = bps;

  // GOP
  int gop = vcodec.H264().keyFrameInterval;
  if (gop <= 0) gop = fps_ * 2;
  ff_->ctx->gop_size = gop;
  ff_->ctx->max_b_frames = 0;  // low latency

  // NVENC private options
  av_opt_set_ptr(ff_->ctx->priv_data, "profile", "high", 0);
  av_opt_set_ptr(ff_->ctx->priv_data, "preset", "p4", 0);  // balanced
  av_opt_set_ptr(ff_->ctx->priv_data, "rc", "cbr_lowdelay_hq", 0);
  av_opt_set_ptr(ff_->ctx->priv_data, "zerolatency", "1", 0);
  av_opt_set_ptr(ff_->ctx->priv_data, "annexb", "1", 0);  // start codes for RTP
  av_opt_set_ptr(ff_->ctx->priv_data, "bf", "0", 0);      // отключить B-frames

  int err = avcodec_open2_ptr(ff_->ctx, ff_->codec, nullptr);
  if (err < 0) {
    char errbuf[256];
    av_strerror(err, errbuf, sizeof(errbuf));
    RTC_LOG(LS_ERROR) << "avcodec_open2 failed: " << errbuf;
    return -1;
  }

  ff_->frame = av_frame_alloc_ptr();
  if (!ff_->frame) return -1;
  ff_->frame->format = ff_->ctx->pix_fmt;
  ff_->frame->width = ff_->ctx->width;
  ff_->frame->height = ff_->ctx->height;
  if (av_frame_get_buffer_ptr(ff_->frame, 32) < 0) return -1;

  return 0;
}

void NVH264Encoder::CloseCodec() {
  if (!ff_) return;
  if (ff_->frame) {
    av_frame_free_ptr(&ff_->frame);
  }
  if (ff_->ctx) {
    avcodec_free_context_ptr(&ff_->ctx);
  }
  ff_.reset();
}

int32_t NVH264Encoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* cb) {
  std::lock_guard<std::mutex> lock(mutex_);
  callback_ = cb;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t NVH264Encoder::Release() {
  std::lock_guard<std::mutex> lock(mutex_);
  initialized_ = false;
  CloseCodec();
  out_buf_.clear();
  callback_ = nullptr;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t NVH264Encoder::Encode(
    const webrtc::VideoFrame& input,
    const std::vector<webrtc::VideoFrameType>* frame_types) {
  // auto start = std::chrono::high_resolution_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_ || !ff_ || !ff_->ctx)
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  if (!callback_) {
    RTC_LOG(LS_WARNING)
        << "InitEncode() has been called, but a callback function "
           "has not been set with RegisterEncodeCompleteCallback()";
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  bool force_key = false;
  if (frame_types) {
    for (auto t : *frame_types)
      if (t == webrtc::VideoFrameType::kVideoFrameKey) force_key = true;
  }

  rtc::scoped_refptr<webrtc::VideoFrameBuffer> buf = input.video_frame_buffer();
  auto i420 = buf->ToI420();

  if (av_frame_make_writable_ptr(ff_->frame) < 0)
    return WEBRTC_VIDEO_CODEC_ERROR;

  CopyPlane(ff_->frame->data[0], ff_->frame->linesize[0], i420->DataY(),
            i420->StrideY(), width_, height_);
  CopyPlane(ff_->frame->data[1], ff_->frame->linesize[1], i420->DataU(),
            i420->StrideU(), width_ / 2, height_ / 2);
  CopyPlane(ff_->frame->data[2], ff_->frame->linesize[2], i420->DataV(),
            i420->StrideV(), width_ / 2, height_ / 2);

  static int64_t pts_counter = 0;
  ff_->frame->pts = pts_counter++;
  ff_->frame->pict_type = force_key ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_NONE;

  int ret = avcodec_send_frame_ptr(ff_->ctx, ff_->frame);
  if (ret < 0) {
    RTC_LOG(LS_ERROR) << "avcodec_send_frame failed: " << ret;
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  ScopedPacket sp;
  while (true) {
    ret = avcodec_receive_packet_ptr(ff_->ctx, sp.pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      std::cout << "error: " << ret << "\n ";
      break;
    }
    if (ret < 0) {
      RTC_LOG(LS_ERROR) << "avcodec_receive_packet failed: " << ret;
      std::cout << "avcodec_receive_packet: " << ret << "\n ";
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    webrtc::EncodedImage encoded;
    encoded.SetRtpTimestamp(input.rtp_timestamp());
    encoded.SetColorSpace(input.color_space());
    encoded._encodedWidth = width_;
    encoded._encodedHeight = height_;
    encoded._frameType = (sp.pkt->flags & AV_PKT_FLAG_KEY)
                             ? webrtc::VideoFrameType::kVideoFrameKey
                             : webrtc::VideoFrameType::kVideoFrameDelta;

    out_buf_.assign(sp.pkt->data, sp.pkt->data + sp.pkt->size);
    encoded.SetEncodedData(
        webrtc::EncodedImageBuffer::Create(out_buf_.data(), out_buf_.size()));

    webrtc::CodecSpecificInfo codec_specific;
    codec_specific.codecType = webrtc::kVideoCodecH264;
    codec_specific.codecSpecific.H264.packetization_mode =
        webrtc::H264PacketizationMode::NonInterleaved;
    codec_specific.codecSpecific.H264.idr_frame =
        encoded._frameType == webrtc::VideoFrameType::kVideoFrameKey;

    callback_->OnEncodedImage(encoded, &codec_specific);
    av_packet_unref_ptr(sp.pkt);
    break;

    // auto end = std::chrono::high_resolution_clock::now();
    // auto duration =
    //     std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // std::cout << "encode: " << duration.count() << " ms\n";
  }

  return WEBRTC_VIDEO_CODEC_OK;
}

void NVH264Encoder::SetRates(const RateControlParameters& p) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!ff_ || !ff_->ctx) return;

  uint32_t kbps = p.bitrate.get_sum_kbps();
  if (kbps == 0) return;
  int64_t bps = static_cast<int64_t>(kbps) * 1000;

  av_opt_set_int_ptr(ff_->ctx->priv_data, "bitrate", bps, 0);
  ff_->ctx->bit_rate = bps;

  if (p.framerate_fps > 0.0) {
    fps_ = static_cast<uint32_t>(p.framerate_fps + 0.5);
    ff_->ctx->time_base = AVRational{1, static_cast<int>(fps_)};
    ff_->ctx->framerate = AVRational{static_cast<int>(fps_), 1};
  }
}

webrtc::VideoEncoder::EncoderInfo NVH264Encoder::GetEncoderInfo() const {
  EncoderInfo info;
  info.implementation_name = "FFmpeg h264_nvenc";
  info.is_hardware_accelerated = true;
  info.supports_native_handle = false;
  info.has_trusted_rate_controller = true;
  info.requested_resolution_alignment = 2;
  info.supports_simulcast = false;
  info.fps_allocation[0].push_back(EncoderInfo::kMaxFramerateFraction);
  return info;
}

void NVH264Encoder::CopyPlane(uint8_t* dst, int dst_stride, const uint8_t* src,
                              int src_stride, int w, int h) {
  for (int y = 0; y < h; ++y) {
    std::memcpy(dst + y * dst_stride, src + y * src_stride, w);
  }
}

}  // namespace libwebrtc