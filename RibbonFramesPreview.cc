#include "RibbonFramesPreview.h"

#include "FrameExtracting.h"
#include "VideoStreamInfoProvider.h"
#include <cassert>

#include <numeric>

RibbonFramesPreview::RibbonFramesPreview(
    std::shared_ptr<FrameExtracting> frame_extractor,
    std::shared_ptr<VideoStreamInfoProvider> video_info)
    : _frame_extractor{std::move(frame_extractor)},
      _video_info{std::move(video_info)} {}

std::expected<std::vector<sf::Sprite>, GenericErrors>
RibbonFramesPreview::GenerateRibbonFramesPreview(size_t frames_in_preview) {
  assert((_rect_size.first != 0) && (_rect_size.second != 0) &&
         "Preview size is not set.");

  const auto video_stream_duration = _video_info->GetVideoStreamDuration();
  if (!video_stream_duration) {
    return std::unexpected{video_stream_duration.error()};
  }

  const double segment_duration =
      video_stream_duration.value() / (frames_in_preview - 1);
  std::vector<Seconds> timestamps(frames_in_preview);
  std::iota(timestamps.begin(), timestamps.end(), 0);
  for (auto &timestamp : timestamps) {
    timestamp = segment_duration * timestamp;
  }

  auto ribbon_frames = _frame_extractor->GetRgbaFramesByTimestamps(timestamps);
  if (!ribbon_frames) {
    return std::unexpected{ribbon_frames.error()};
  }

  _ribbon_textures.clear();
  _ribbon_textures.reserve(frames_in_preview);

  std::vector<sf::Sprite> ribbon_sprites{};
  ribbon_sprites.reserve(frames_in_preview);

  const auto frame_size = _video_info->GetFrameSize();
  if (!frame_size) {
    return std::unexpected{frame_size.error()};
  }

  for (auto ribbon_frame = ribbon_frames->begin();
       ribbon_frame != ribbon_frames->end(); ++ribbon_frame) {
    auto &texture = _ribbon_textures.emplace_back(
        sf::Vector2u{frame_size->first, frame_size->second});
    texture.update(
        ribbon_frame->get()->data[0],
        sf::Vector2u{static_cast<unsigned int>(ribbon_frame->get()->width),
                     static_cast<unsigned int>(ribbon_frame->get()->height)},
        sf::Vector2u{0, 0});

    auto &ribbon_sprite = ribbon_sprites.emplace_back(texture);

    const auto texture_size = texture.getSize();

    ribbon_sprite.setScale(
        {(static_cast<float>(_rect_size.first) / frames_in_preview) /
             texture_size.x,
         static_cast<float>(_rect_size.second) / texture_size.y});

    const auto index = std::distance(ribbon_frames->begin(), ribbon_frame);

    const float sprite_width =
        static_cast<float>(_rect_size.first) / frames_in_preview;
    const float sprite_pos_x = _rect_position.first + index * sprite_width;
    const float sprite_pos_y = static_cast<float>(_rect_position.second);

    ribbon_sprite.setPosition({sprite_pos_x, sprite_pos_y});
  }

  return ribbon_sprites;
}