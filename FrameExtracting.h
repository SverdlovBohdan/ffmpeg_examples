#pragma once

#include "Errors.h"
#include "FfmpegSmartPtrs.h"
#include <Types.h>
#include <expected>
#include <vector>

class FrameExtracting {
 public:
  virtual ~FrameExtracting() = default;

  virtual std::expected<AvFrameUniquePtr, GenericErrors>
  GetOriginalFrame(AvFrameUniquePtr pre_allocated_frame) = 0;

  virtual std::expected<AvFrameUniquePtr, GenericErrors>
  GetRgbaFrame(AvFrameUniquePtr pre_allocated_frame) = 0;

  virtual std::expected<std::vector<AvFrameUniquePtr>, GenericErrors>
  GetRgbaFramesByTimestamps(const std::vector<Seconds> timestamps) = 0;
};