#include "FrameExtractorFromFile.h"

#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

FrameExtractorFromFile::FrameExtractorFromFile(std::filesystem::path file)
    : _file{std::move(file)},
      _format_ctx{nullptr},
      _codec_ctx{nullptr},
      _sws_ctx{nullptr},
      _video_stream_idx{-1},
      _packet{nullptr, [](auto*) {}} {}

FrameExtractorFromFile::~FrameExtractorFromFile() {
  avcodec_free_context(&_codec_ctx);
  sws_freeContext(_sws_ctx);
  avformat_close_input(&_format_ctx);
}

AvFrameUniquePtr FrameExtractorFromFile::GetOriginalFrame() {
  if (!IsReady() && OpenCodec()) {
    return AvFrameUniquePtr{nullptr, [](auto*) {}};
  }

  if (!_packet) {
    _packet = MakeAvPacketUnique();
  }

  return MakeAvFrameUnique();
}

AvFrameUniquePtr FrameExtractorFromFile::GetRgbaFrame() {
  if (!IsReady() && OpenCodec()) {
    return AvFrameUniquePtr{nullptr, [](auto*) {}};
  }

  if (!_packet) {
    _packet = MakeAvPacketUnique();
  }

  AVCodecParameters* codec_params =
      _format_ctx->streams[_video_stream_idx]->codecpar;

  uint8_t* buffer =
      reinterpret_cast<uint8_t*>(av_malloc(av_image_get_buffer_size(
          AV_PIX_FMT_RGBA, codec_params->width, codec_params->height, 1)));
  AvFrameUniquePtr rgb_frame =
      AvFrameUniquePtr{av_frame_alloc(), [&buffer](AVFrame* ptr) {
                         if (ptr) {
                           av_frame_free(&ptr);
                         }
                         if (buffer) {
                           av_free(buffer);
                           buffer = nullptr;
                         }
                       }};

  av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer,
                       AV_PIX_FMT_RGBA, codec_params->width,
                       codec_params->height, 1);

  AvFrameUniquePtr frame = MakeAvFrameUnique();

  if (av_read_frame(_format_ctx, _packet.get()) >= 0) {
    if (_packet->stream_index == _video_stream_idx) {
      if (avcodec_send_packet(_codec_ctx, _packet.get()) == 0) {
        while (avcodec_receive_frame(_codec_ctx, frame.get()) == 0) {
          std::cout << "Decoded frame: "
                    << "Width=" << frame->width << ", Height=" << frame->height
                    << ", Format=" << frame->format << std::endl;
          // Convert frame to RGBA
          sws_scale(_sws_ctx, frame->data, frame->linesize, 0,
                    codec_params->height, rgb_frame->data, rgb_frame->linesize);
        }
      }
    }

    av_packet_unref(_packet.get());
  }

  return rgb_frame;
}

bool FrameExtractorFromFile::OpenCodec() {
  if (avformat_open_input(&_format_ctx, _file.c_str(), nullptr, nullptr) != 0) {
    std::cerr << "Could not open file: " << _file << std::endl;
    return false;
  }

  if (avformat_find_stream_info(_format_ctx, nullptr) < 0) {
    std::cerr << "Could not retreive stream info." << std::endl;
    avformat_close_input(&_format_ctx);
    return false;
  }

  _video_stream_idx = -1;
  for (size_t i = 0; i < _format_ctx->nb_streams; ++i) {
    if (_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      _video_stream_idx = i;
      break;
    }
  }

  if (_video_stream_idx == -1) {
    std::cerr << "No video stream found in the file" << std::endl;
    avformat_close_input(&_format_ctx);
    return false;
  }

  AVCodecParameters* codec_params =
      _format_ctx->streams[_video_stream_idx]->codecpar;
  const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
  if (!codec) {
    std::cerr << "Unsupported codec!" << std::endl;
    avformat_close_input(&_format_ctx);
    return false;
  }

  _codec_ctx = avcodec_alloc_context3(codec);
  if (!_codec_ctx) {
    std::cerr << "Could not allocate codec context." << std::endl;
    avformat_close_input(&_format_ctx);
    return false;
  }

  if (avcodec_parameters_to_context(_codec_ctx, codec_params) < 0) {
    std::cerr << "Failed to copy codec parameters to codec context."
              << std::endl;
    avcodec_free_context(&_codec_ctx);
    avformat_close_input(&_format_ctx);
    return false;
  }

  if (avcodec_open2(_codec_ctx, codec, nullptr) < 0) {
    std::cerr << "Could not open codec." << std::endl;
    avcodec_free_context(&_codec_ctx);
    avformat_close_input(&_format_ctx);
    return false;
  }

  _sws_ctx = sws_getContext(codec_params->width, codec_params->height,
                            _codec_ctx->pix_fmt, codec_params->width,
                            codec_params->height, AV_PIX_FMT_RGBA, SWS_BILINEAR,
                            nullptr, nullptr, nullptr);
  if (!_sws_ctx) {
    std::cerr << "Can not get sws context." << std::endl;
    return false;
  }

  return true;
}

bool FrameExtractorFromFile::IsReady() const {
  return _format_ctx && _codec_ctx && _sws_ctx;
}