#pragma once

#include <filesystem>
#include <optional>

#include "FrameExtracting.h"
#include "VideoStreamInfoProvider.h"

struct AVFormatContext;
struct AVCodecContext;
struct SwsContext;

class VideoStream : public FrameExtracting, public VideoStreamInfoProvider {
public:
  explicit VideoStream(std::filesystem::path file);

  ~VideoStream() override;

  std::expected<AvFrameUniquePtr, GenericErrors>
  GetOriginalFrame(AvFrameUniquePtr pre_allocated_frame) override;

  std::expected<AvFrameUniquePtr, GenericErrors>
  GetRgbaFrame(AvFrameUniquePtr pre_allocated_frame) override;

  std::expected<std::vector<AvFrameUniquePtr>, GenericErrors>
  GetRgbaFramesByTimestamps(const std::vector<Seconds> timestamps) override;

  std::expected<RectSize, GenericErrors> GetFrameSize() override;
  std::expected<Seconds, GenericErrors> GetVideoStreamDuration() override;

private:
  bool OpenCodec();
  bool IsReady() const;
  void SetVideoStreamInfo();

  std::filesystem::path _file;

  AVFormatContext* _format_ctx;
  AVCodecContext* _codec_ctx;
  SwsContext* _sws_ctx;
  int _video_stream_idx;

  AvPacketUniquePtr _packet;
  AvFrameUniquePtr _original_frame_cache;

  mutable std::optional<RectSize> _frame_size_cache;
  mutable std::optional<Seconds> _video_stream_duration;
};