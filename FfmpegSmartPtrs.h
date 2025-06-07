#pragma once

#include <functional>
#include <memory>

extern "C" {
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
}

using AvFrameUniquePtr =
    std::unique_ptr<AVFrame, std::function<void(AVFrame *)>>;
using AvPacketUniquePtr =
    std::unique_ptr<AVPacket, std::function<void(AVPacket *)>>;

AvFrameUniquePtr MakeAvFrameUnique(AVFrame *frame = av_frame_alloc());
AvPacketUniquePtr MakeAvPacketUnique(AVPacket *packet = av_packet_alloc());