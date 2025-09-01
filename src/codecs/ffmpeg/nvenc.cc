#include "src/codecs/ffmpeg/nvenc.h"
#include "src/codecs/ffmpeg/nvenc_encoder.h"
#include "absl/base/nullability.h"

#include "rtc_base/logging.h"

namespace libwebrtc {

absl_nonnull std::unique_ptr<webrtc::VideoEncoder> CreateNVH264Encoder(
    const webrtc::Environment& env,
    const webrtc::SdpVideoFormat& format) {
#if defined(WEBRTC_USE_H264)
  RTC_LOG(LS_INFO) << "Creating NVH264EncoderImpl.";
  return std::make_unique<NVH264Encoder>();
#else
  RTC_CHECK_NOTREACHED();
#endif
}


}  // namespace webrtc
