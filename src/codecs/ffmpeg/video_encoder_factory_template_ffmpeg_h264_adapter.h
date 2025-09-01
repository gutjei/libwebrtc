#ifndef API_VIDEO_CODECS_VIDEO_ENCODER_FACTORY_TEMPLATE_NVENC_H264_ADAPTER_H_
#define API_VIDEO_CODECS_VIDEO_ENCODER_FACTORY_TEMPLATE_NVENC_H264_ADAPTER_H_

#include <memory>
#include <vector>

#include "api/environment/environment.h"
#include "api/video_codecs/scalability_mode.h"
#include "api/video_codecs/sdp_video_format.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "api/video_codecs/video_encoder.h"
#include "src/codecs/ffmpeg/nvenc.h"

namespace libwebrtc {

struct NVH264EncoderTemplateAdapter {
 static std::vector<webrtc::SdpVideoFormat> SupportedFormats() {
   return webrtc::SupportedH264Codecs(/*add_scalability_modes=*/true);
  }

  static std::unique_ptr<webrtc::VideoEncoder> CreateEncoder(
      [[maybe_unused]] const webrtc::Environment& env,
      [[maybe_unused]] const webrtc::SdpVideoFormat& format) {
    return CreateNVH264Encoder(env, format);
  }

  static bool IsScalabilityModeSupported(
      [[maybe_unused]] webrtc::ScalabilityMode scalability_mode) {
    return webrtc::H264Encoder::SupportsScalabilityMode(scalability_mode);
  }
};

}  // namespace webrtc

#endif  // API_VIDEO_CODECS_VIDEO_ENCODER_FACTORY_TEMPLATE_NVENC_H264_ADAPTER_H_
