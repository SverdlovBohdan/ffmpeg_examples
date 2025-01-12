#include "FfmpegSmartPtrs.h"

AvFrameUniquePtr MakeAvFrameUnique() {
  return AvFrameUniquePtr{av_frame_alloc(), [](AVFrame* ptr) {
                            if (ptr) {
                              av_frame_free(&ptr);
                            }
                          }};
}

AvPacketUniquePtr MakeAvPacketUnique() {
  return AvPacketUniquePtr{av_packet_alloc(), [](AVPacket* ptr) {
                             if (ptr) {
                               av_packet_free(&ptr);
                             }
                           }};
}