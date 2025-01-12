#pragma once

#include "FfmpegSmartPtrs.h"

class FrameExtracting {
 public:
  virtual ~FrameExtracting() = default;

  virtual AvFrameUniquePtr GetOriginalFrame() = 0;

  virtual AvFrameUniquePtr GetRgbaFrame() = 0;
};