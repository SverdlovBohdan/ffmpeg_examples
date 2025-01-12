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

  AvFrameUniquePtr GetOriginalFrame() override;

  AvFrameUniquePtr GetRgbaFrame() override;

 private:
  bool OpenCodec();
  bool IsReady() const;

  std::filesystem::path _file;

  AVFormatContext* _format_ctx;
  AVCodecContext* _codec_ctx;
  SwsContext* _sws_ctx;
  int _video_stream_idx;

  AvPacketUniquePtr _packet;
};