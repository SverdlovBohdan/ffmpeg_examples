#include "FfmpegSmartPtrs.h"

AvFrameUniquePtr MakeAvFrameUnique(AVFrame *frame) {
  return AvFrameUniquePtr{frame, [](AVFrame *frame) { av_frame_free(&frame); }};
}

AvPacketUniquePtr MakeAvPacketUnique(AVPacket *packet) {
  return AvPacketUniquePtr{packet,
                           [](AVPacket *packet) { av_packet_free(&packet); }};
}