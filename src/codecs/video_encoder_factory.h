#ifndef OWT_BASE_WIN_MSDKVIDEOENCODER_FACTORY_H_
#define OWT_BASE_WIN_MSDKVIDEOENCODER_FACTORY_H_

#include <vector>

#include "api/video/video_codec_type.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "src/codecs/video_encoder_factory.h"

namespace libwebrtc {

class HWController {
 public:
  void SetUseHW(bool use_hw) { use_hw_ = use_hw; }
  bool UseHW() { return use_hw_; }

 private:
  bool use_hw_;
};

class BetterEncoderFactory : public webrtc::VideoEncoderFactory {
 public:
  BetterEncoderFactory(std::shared_ptr<HWController> controller) 
      :controller_(controller) {}
  ~BetterEncoderFactory() {}
  std::unique_ptr<webrtc::VideoEncoder> Create(
      const webrtc::Environment& env,
      const webrtc::SdpVideoFormat& format) override;

  std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

  CodecSupport QueryCodecSupport(
      const webrtc::SdpVideoFormat& format,
      std::optional<std::string> scalability_mode) const override;

 private:
  std::shared_ptr<HWController> controller_;
};
}  // namespace libwebrtc
#endif  // OWT_BASE_WIN_MSDKVIDEOENCODER_FACTORY_H_