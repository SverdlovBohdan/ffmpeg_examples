#pragma once

#include <expected>
#include "Types.h"
#include "Errors.h"

class VideoStreamInfoProvider {
    public:
    virtual ~VideoStreamInfoProvider() = default;

    virtual std::expected<RectSize, GenericErrors> GetFrameSize() const = 0;
    virtual std::expected<Seconds, GenericErrors> GetVideoStreamDuration() const = 0;
};