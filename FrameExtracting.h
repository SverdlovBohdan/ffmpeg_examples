#pragma once

#include "FfmpegSmartPtrs.h"

class FrameExtracting {
 public:
  virtual ~FrameExtracting() = default;

  virtual AvFrameUniquePtr GetOriginalFrame(
      AvFrameUniquePtr pre_allocated_frame) = 0;

  virtual AvFrameUniquePtr GetRgbaFrame(
      AvFrameUniquePtr pre_allocated_frame) = 0;

  virtual bool HasMoreVideoFrames() const = 0;
};