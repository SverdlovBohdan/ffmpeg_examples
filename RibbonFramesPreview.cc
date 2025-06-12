#include "RibbonFramesPreview.h"

RibbonFramesPreview::RibbonFramesPreview(
    std::shared_ptr<FrameExtracting> frame_extractor)
    : _frame_extractor{std::move(frame_extractor)} {}