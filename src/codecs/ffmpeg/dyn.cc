#include "src/codecs/ffmpeg/dyn.h"

#include <windows.h>

#include <iostream>
#include <string>

#include "rtc_base/logging.h"

namespace ffmpeg_dyn {

bool load_avcodec(HMODULE hAvcodec) {
  av_packet_alloc_ptr =
      (av_packet_alloc_t)GetProcAddress(hAvcodec, "av_packet_alloc");
  av_packet_free_ptr =
      (av_packet_free_t)GetProcAddress(hAvcodec, "av_packet_free");
  avcodec_find_encoder_by_name_ptr =
      (avcodec_find_encoder_by_name_t)GetProcAddress(
          hAvcodec, "avcodec_find_encoder_by_name");
  avcodec_alloc_context3_ptr = (avcodec_alloc_context3_t)GetProcAddress(
      hAvcodec, "avcodec_alloc_context3");
  avcodec_open2_ptr =
      (avcodec_open2_t)GetProcAddress(hAvcodec, "avcodec_open2");
  avcodec_free_context_ptr =
      (avcodec_free_context_t)GetProcAddress(hAvcodec, "avcodec_free_context");
  avcodec_send_frame_ptr =
      (avcodec_send_frame_t)GetProcAddress(hAvcodec, "avcodec_send_frame");
  avcodec_receive_packet_ptr = (avcodec_receive_packet_t)GetProcAddress(
      hAvcodec, "avcodec_receive_packet");
  av_packet_unref_ptr =
      (av_packet_unref_t)GetProcAddress(hAvcodec, "av_packet_unref");

  if (!av_packet_alloc_ptr || !av_packet_free_ptr ||
      !avcodec_find_encoder_by_name_ptr || !avcodec_alloc_context3_ptr ||
      !avcodec_open2_ptr || !avcodec_free_context_ptr ||
      !avcodec_send_frame_ptr || !avcodec_receive_packet_ptr ||
      !av_packet_unref_ptr) {
    RTC_LOG(LS_ERROR)
        << "av_packet_alloc: " << (av_packet_alloc_ptr ? "OK" : "FAIL") << "\n"
        << "av_packet_free: " << (av_packet_free_ptr ? "OK" : "FAIL") << "\n"
        << "avcodec_find_encoder_by_name: "
        << (avcodec_find_encoder_by_name_ptr ? "OK" : "FAIL") << "\n"
        << "avcodec_alloc_context3: "
        << (avcodec_alloc_context3_ptr ? "OK" : "FAIL") << "\n"
        << "avcodec_open2: " << (avcodec_open2_ptr ? "OK" : "FAIL") << "\n"
        << "avcodec_free_context: "
        << (avcodec_free_context_ptr ? "OK" : "FAIL") << "\n"
        << "avcodec_send_frame: " << (avcodec_send_frame_ptr ? "OK" : "FAIL")
        << "\n"
        << "avcodec_receive_packet: "
        << (avcodec_receive_packet_ptr ? "OK" : "FAIL") << "\n"
        << "av_packet_unref: " << (av_packet_unref_ptr ? "OK" : "FAIL");
    FreeLibrary(hAvcodec);
    return false;
  }

  return true;
}

bool load_avutil(HMODULE hAvcodec) {
  av_opt_set_ptr = (av_opt_set_t)GetProcAddress(hAvcodec, "av_opt_set");
  av_frame_alloc_ptr =
      (av_frame_alloc_t)GetProcAddress(hAvcodec, "av_frame_alloc");
  av_frame_get_buffer_ptr =
      (av_frame_get_buffer_t)GetProcAddress(hAvcodec, "av_frame_get_buffer");
  av_frame_free_ptr =
      (av_frame_free_t)GetProcAddress(hAvcodec, "av_frame_free");
  av_frame_make_writable_ptr = (av_frame_make_writable_t)GetProcAddress(
      hAvcodec, "av_frame_make_writable");
  av_opt_set_int_ptr =
      (av_opt_set_int_t)GetProcAddress(hAvcodec, "av_opt_set_int");

  if (!av_opt_set_ptr || !av_frame_alloc_ptr || !av_frame_get_buffer_ptr ||
      !av_frame_free_ptr || !av_frame_make_writable_ptr) {
    RTC_LOG(LS_ERROR)

        << "av_opt_set: " << (av_opt_set_ptr ? "OK" : "FAIL") << "\n"
        << "av_frame_alloc: " << (av_frame_alloc_ptr ? "OK" : "FAIL") << "\n"
        << "av_frame_get_buffer: " << (av_frame_get_buffer_ptr ? "OK" : "FAIL")
        << "\n"
        << "av_frame_free: " << (av_frame_free_ptr ? "OK" : "FAIL") << "\n"
        << "av_frame_make_writable: "
        << (av_frame_make_writable_ptr ? "OK" : "FAIL") << "\n"
        << "av_opt_set_int: " << (av_opt_set_int_ptr ? "OK" : "FAIL");
    FreeLibrary(hAvcodec);
    return false;
  }

  return true;
}

bool init_avcodec() {
  static bool initialized = false;
  static bool success = false;

  if (!initialized) {
    initialized = true;

    HMODULE hAvcodec = LoadLibraryA("avcodec-61.dll");
    if (hAvcodec) {
      success = load_avcodec(hAvcodec);
    }
    if (!success) {
      RTC_LOG(LS_ERROR) << "open avcodec-61.dll failed";
      return false;
    }

    HMODULE hAvutil = LoadLibraryA("avutil-59.dll");
    if (hAvutil) {
      success = load_avutil(hAvutil);
    }
    if (!success) {
      RTC_LOG(LS_ERROR) << "open avutil-59.dll failed";
      return false;
    }
  }
  return success;
}

}  // namespace ffmpeg_dyn