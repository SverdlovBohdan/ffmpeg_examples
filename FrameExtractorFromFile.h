#pragma once

#include <filesystem>

#include "FrameExtracting.h"

struct AVFormatContext;
struct AVCodecContext;
struct SwsContext;

class FrameExtractorFromFile : public FrameExtracting {
 public:
  explicit FrameExtractorFromFile(std::filesystem::path file);

  ~FrameExtractorFromFile() override;

  AvFrameUniquePtr GetOriginalFrame(
      AvFrameUniquePtr pre_allocated_frame) override;

  AvFrameUniquePtr GetRgbaFrame(AvFrameUniquePtr pre_allocated_frame) override;

  bool HasMoreVideoFrames() const override;

 private:
  bool OpenCodec();
  bool IsReady() const;

  std::filesystem::path _file;

  AVFormatContext* _format_ctx;
  AVCodecContext* _codec_ctx;
  SwsContext* _sws_ctx;
  int _video_stream_idx;

  AvPacketUniquePtr _packet;
  AvFrameUniquePtr _original_frame_cache;
  bool _has_more_video_frames;
};