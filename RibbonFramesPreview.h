#pragma once

#include "Errors.h"
#include "Types.h"
#include <SFML/Graphics.hpp>
#include <expected>
#include <memory>
#include <vector>

class FrameExtracting;
class VideoStreamInfoProvider;

class RibbonFramesPreview {
public:
  RibbonFramesPreview(std::shared_ptr<FrameExtracting> frame_extractor,
                      std::shared_ptr<VideoStreamInfoProvider> video_info);

  void SetRibbonSize(const RectSize &rect_size) { _rect_size = rect_size; }
  void SetRibbonPosition(const RectSize &rect_position) {
    _rect_position = rect_position;
  }

  std::expected<std::vector<sf::Sprite>, GenericErrors>
  GenerateRibbonFramesPreview(size_t frames_in_preview);

private:
  std::shared_ptr<FrameExtracting> _frame_extractor;
  std::shared_ptr<VideoStreamInfoProvider> _video_info;

  RectSize _rect_size{0, 0};
  RectSize _rect_position{0, 0};
  std::vector<sf::Texture> _ribbon_textures{};
};