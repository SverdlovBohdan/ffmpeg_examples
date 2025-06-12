#pragma once

#include <expected>
#include "Types.h"
#include "Errors.h"

class VideoStreamInfoProvider {
    public:
    virtual ~VideoStreamInfoProvider() = default;

    virtual std::expected<RectSize, GenericErrors> GetFrameSize() = 0;
    virtual std::expected<Seconds, GenericErrors> GetVideoStreamDuration() = 0;
};