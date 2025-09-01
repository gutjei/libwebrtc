// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "src/codecs/video_encoder_factory.h"

#include <string>

#include "absl/strings/match.h"
#include "api/environment/environment.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/video_encoder_factory_template.h"
#include "src/codecs/ffmpeg/nvenc.h"
#include "src/codecs/ffmpeg/nvenc_encoder.h"
#if defined(RTC_USE_LIBAOM_AV1_ENCODER)
#include "api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h"  // nogncheck
#endif
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h"
#if defined(WEBRTC_USE_H264)
#include "src/codecs/ffmpeg/video_encoder_factory_template_ffmpeg_h264_adapter.h"  // nogncheck
#include "api/video_codecs/video_encoder_factory_template_open_h264_adapter.h"  // nogncheck
#endif

namespace libwebrtc {

using FactorySW = webrtc::VideoEncoderFactoryTemplate<
    webrtc::LibvpxVp8EncoderTemplateAdapter,
#if defined(WEBRTC_USE_H264)
    webrtc::OpenH264EncoderTemplateAdapter, 
#endif
#if defined(RTC_USE_LIBAOM_AV1_ENCODER)
    webrtc::LibaomAv1EncoderTemplateAdapter,
#endif
    webrtc::LibvpxVp9EncoderTemplateAdapter>;

using FactoryHW = webrtc::VideoEncoderFactoryTemplate<NVH264EncoderTemplateAdapter>;


std::vector<webrtc::SdpVideoFormat> BetterEncoderFactory::GetSupportedFormats()
    const {
  return FactorySW().GetSupportedFormats();
}

std::unique_ptr<webrtc::VideoEncoder> BetterEncoderFactory::Create(
    const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) {
  if (controller_->UseHW()) {
    auto original_format =
        FuzzyMatchSdpVideoFormat(FactoryHW().GetSupportedFormats(), format);
    return original_format ? FactoryHW().Create(env, *original_format)
                          : nullptr;
  }
  auto original_format =
      FuzzyMatchSdpVideoFormat(FactorySW().GetSupportedFormats(), format);
  return original_format ? FactorySW().Create(env, *original_format) : nullptr;
}

webrtc::VideoEncoderFactory::CodecSupport BetterEncoderFactory::QueryCodecSupport(
    const webrtc::SdpVideoFormat& format,
    std::optional<std::string> scalability_mode) const {
  if (controller_->UseHW()) {
    auto original_format = webrtc::FuzzyMatchSdpVideoFormat(
        FactoryHW().GetSupportedFormats(), format);
    return original_format
               ? FactoryHW().QueryCodecSupport(*original_format, scalability_mode)
               : VideoEncoderFactory::CodecSupport{.is_supported = false};
  }
  auto original_format =
      webrtc::FuzzyMatchSdpVideoFormat(FactorySW().GetSupportedFormats(), format);
  return original_format
             ? FactorySW().QueryCodecSupport(*original_format, scalability_mode)
             : VideoEncoderFactory::CodecSupport{.is_supported = false};
}

}  // namespace libwebrtc
