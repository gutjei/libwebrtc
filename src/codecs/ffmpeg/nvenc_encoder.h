#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/sdp_video_format.h"

namespace libwebrtc {


// Forward-declare to avoid leaking FFmpeg headers in users of the encoder.
struct FFmpegState;

class NVH264Encoder : public webrtc::VideoEncoder {
    public:
        NVH264Encoder();
        ~NVH264Encoder() override;

        int32_t InitEncode(const webrtc::VideoCodec* codec_settings,
                           const Settings& settings) override;
        int32_t RegisterEncodeCompleteCallback(
            webrtc::EncodedImageCallback* callback) override;
        int32_t Release() override;
        int32_t Encode(const webrtc::VideoFrame& frame,
            const std::vector<webrtc::VideoFrameType>* frame_types) override;
        void SetRates(const RateControlParameters& parameters) override;
        EncoderInfo GetEncoderInfo() const override;

    private:
        static void CopyPlane(uint8_t* dst, int dst_stride, const uint8_t* src, int src_stride, int w, int h);

        int OpenCodec(const webrtc::VideoCodec& codec);
        void CloseCodec();

        std::mutex mutex_;
        bool initialized_ = false;

        webrtc::VideoCodec codec_{};
        int width_ = 0;
        int height_ = 0;
        uint32_t fps_ = 30;

        std::unique_ptr<FFmpegState> ff_;

        webrtc::EncodedImageCallback* callback_ = nullptr;

        std::vector<uint8_t> out_buf_;
};


} // namespace webrtc