#include "FrameExtractorFromFile.h"

#include <format>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

FrameExtractorFromFile::FrameExtractorFromFile(std::filesystem::path file)
    : _file{std::move(file)}, _format_ctx{nullptr}, _codec_ctx{nullptr},
      _sws_ctx{nullptr}, _video_stream_idx{-1},
      _packet{MakeAvPacketUnique(nullptr)},
      _original_frame_cache{MakeAvFrameUnique(nullptr)} {}

FrameExtractorFromFile::~FrameExtractorFromFile() {
  avcodec_free_context(&_codec_ctx);
  sws_freeContext(_sws_ctx);
  _sws_ctx = nullptr;
  avformat_close_input(&_format_ctx);
}

std::expected<AvFrameUniquePtr, GenericErrors>
FrameExtractorFromFile::GetOriginalFrame(AvFrameUniquePtr pre_allocated_frame) {
  if (!IsReady() && !OpenCodec()) {
    return std::unexpected(GenericErrors::kCodecError);
  }

  if (!_packet) {
    _packet = MakeAvPacketUnique();
  }

  AVCodecParameters* codec_params =
      _format_ctx->streams[_video_stream_idx]->codecpar;

  if (!pre_allocated_frame) {
    pre_allocated_frame = MakeAvFrameUnique();
  }

  bool found = false;
  while (av_read_frame(_format_ctx, _packet.get()) >= 0 && !found) {
    if (_packet->stream_index == _video_stream_idx) {
      if (avcodec_send_packet(_codec_ctx, _packet.get()) == 0) {
        while (avcodec_receive_frame(_codec_ctx, pre_allocated_frame.get()) ==
               0) {
          std::cout << "Decoded frame: "
                    << "Width=" << pre_allocated_frame->width
                    << ", Height=" << pre_allocated_frame->height
                    << ", Format=" << pre_allocated_frame->format << std::endl;
          found = true;
          av_packet_unref(_packet.get());
          return pre_allocated_frame;
        }
      }
    }

    av_packet_unref(_packet.get());
  }

  return std::unexpected(GenericErrors::kNoMoreFrames);
}

std::expected<AvFrameUniquePtr, GenericErrors>
FrameExtractorFromFile::GetRgbaFrame(AvFrameUniquePtr pre_allocated_frame) {
  auto maybe_original_frame =
      GetOriginalFrame(std::move(_original_frame_cache));
  if (!maybe_original_frame.has_value()) {
    return maybe_original_frame;
  }

  _original_frame_cache = *std::move(maybe_original_frame);

  AVCodecParameters* codec_params =
      _format_ctx->streams[_video_stream_idx]->codecpar;

  if (!pre_allocated_frame) {
    auto raw_preallocated_frame = av_frame_alloc();
    av_image_alloc(raw_preallocated_frame->data,
                   raw_preallocated_frame->linesize, codec_params->width,
                   codec_params->height, AV_PIX_FMT_RGBA, 16);

    pre_allocated_frame =
        AvFrameUniquePtr{raw_preallocated_frame, [](AVFrame *frame) mutable {
                           av_freep(&frame->data);
                           av_frame_free(&frame);
                         }};
    pre_allocated_frame->width = codec_params->width;
    pre_allocated_frame->height = codec_params->height;
    pre_allocated_frame->format = codec_params->format;

    _sws_ctx = sws_getContext(codec_params->width, codec_params->height,
                              _codec_ctx->pix_fmt, codec_params->width,
                              codec_params->height, AV_PIX_FMT_RGBA,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!_sws_ctx) {
      std::cerr << "Can not get sws context." << std::endl;
      return std::unexpected(GenericErrors::kFrameScaleError);
    }
  }

  if (sws_scale(_sws_ctx, _original_frame_cache->data,
                _original_frame_cache->linesize, 0, codec_params->height,
                pre_allocated_frame->data,
                pre_allocated_frame->linesize) == 0) {
    return std::unexpected(GenericErrors::kFrameScaleError);
  }

  return pre_allocated_frame;
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

  return true;
}

bool FrameExtractorFromFile::IsReady() const {
  return _format_ctx && _codec_ctx;
}

std::expected<RectSize, GenericErrors>
FrameExtractorFromFile::GetFrameSize() const {
  SetVideoStreamInfo();

  if (!_frame_size_cache) {
    return std::unexpected{GenericErrors::kNoVideoInfo};
  }

  return *_frame_size_cache;
}

std::expected<Seconds, GenericErrors>
FrameExtractorFromFile::GetVideoStreamDuration() const {
  SetVideoStreamInfo();

  if (!_video_stream_duration) {
    return std::unexpected{GenericErrors::kNoVideoInfo};
  }

  return *_video_stream_duration;
}

void FrameExtractorFromFile::SetVideoStreamInfo() const {
  if (_frame_size_cache && _video_stream_duration) {
    return;
  }

  FrameExtractorFromFile video_stream_info_extractor{_file};
  if (auto maybe_frame = video_stream_info_extractor.GetOriginalFrame(
          MakeAvFrameUnique(nullptr));
      maybe_frame.has_value()) {
    _frame_size_cache =
        std::make_pair(static_cast<unsigned int>(maybe_frame.value()->width),
                       static_cast<unsigned int>(maybe_frame.value()->height));

    _video_stream_duration = static_cast<double>(
        video_stream_info_extractor._format_ctx
            ->streams[video_stream_info_extractor._video_stream_idx]
            ->duration *
        av_q2d(video_stream_info_extractor._format_ctx
                   ->streams[video_stream_info_extractor._video_stream_idx]
                   ->time_base));
  } else {
    std::cout << "Can't get video stream info." << std::endl;
  }
}