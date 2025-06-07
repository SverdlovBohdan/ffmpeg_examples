#pragma once

#include "Errors.h"
#include "FfmpegSmartPtrs.h"
#include <expected>

class FrameExtracting {
 public:
  virtual ~FrameExtracting() = default;

  virtual std::expected<AvFrameUniquePtr, GenericErrors>
  GetOriginalFrame(AvFrameUniquePtr pre_allocated_frame) = 0;

  virtual std::expected<AvFrameUniquePtr, GenericErrors>
  GetRgbaFrame(AvFrameUniquePtr pre_allocated_frame) = 0;
};