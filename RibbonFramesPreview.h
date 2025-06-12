#pragma once

#include "Types.h"
#include <memory>

class FrameExtracting;

class RibbonFramesPreview {
public:
  explicit RibbonFramesPreview(
      std::shared_ptr<FrameExtracting> frame_extractor);

  void SetRibbonSize(const RectSize &rect_size) { _rect_size = rect_size; }

private:
  std::shared_ptr<FrameExtracting> _frame_extractor;
  RectSize _rect_size;
};