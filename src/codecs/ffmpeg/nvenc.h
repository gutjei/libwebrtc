#ifndef FFMPEG_NVENC_H264_ENCODER_FACTORY_H_
#define FFMPEG_NVENC_H264_ENCODER_FACTORY_H_

#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/environment/environment.h"
#include "api/video_codecs/video_encoder.h"
#include "absl/base/nullability.h"


#include <memory>
#include <vector>

namespace libwebrtc {

absl_nonnull std::unique_ptr<webrtc::VideoEncoder> CreateNVH264Encoder(
    const webrtc::Environment & env, const webrtc::SdpVideoFormat& format);

}  // namespace webrtc

#endif  // FFMPEG_NVENC_H264_ENCODER_FACTORY_H_