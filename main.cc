#include <SFML/Graphics.hpp>
#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <numeric>

#include "VideoStream.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace {
constexpr unsigned int kRibbonNavigatorHeight = 100;
}

int main() {
  const std::filesystem::path kBunnyFile =
      "./resources/BigBuckBunny36010s5MB.mp4";

  sf::Vector2u window_size{};
  auto video_stream = std::make_shared<VideoStream>(kBunnyFile);

  if (auto maybe_size = video_stream->GetFrameSize(); maybe_size.has_value()) {
    window_size = {static_cast<unsigned int>(maybe_size.value().first),
                   static_cast<unsigned int>(maybe_size.value().second) +
                       kRibbonNavigatorHeight};
  } else {
    std::cout << "Can't get frame info." << std::endl;
    return 0;
  }

  // create the window
  sf::RenderWindow window(sf::VideoMode(window_size), "My window");

  // Create an empty texture
  sf::Texture texture(
      sf::Vector2u{window_size.x, window_size.y - kRibbonNavigatorHeight});

  // Create a sprite that will display the texture
  sf::Sprite sprite(texture);

  auto ribbon_frames_extractor = std::make_shared<VideoStream>(kBunnyFile);

  const auto segments_count = 16;
  const double segment_duration =
      ribbon_frames_extractor->GetVideoStreamDuration().value() /
      (segments_count - 1);
  std::vector<Seconds> timestamps(segments_count);
  std::iota(timestamps.begin(), timestamps.end(), 0);
  for (auto &timestamp : timestamps) {
    timestamp = segment_duration * timestamp;
    std::cout << timestamp << std::endl;
  }

  auto ribbon_frames =
      ribbon_frames_extractor->GetRgbaFramesByTimestamps(timestamps).value();

  std::vector<sf::Texture> ribbon_textures{};
  ribbon_textures.reserve(segments_count);

  std::vector<sf::Sprite> ribbon_sprites{};
  ribbon_sprites.reserve(segments_count);

  for (auto ribbon_frame = ribbon_frames.begin();
       ribbon_frame != ribbon_frames.end(); ++ribbon_frame) {
    ribbon_textures.emplace_back(
        sf::Vector2u{window_size.x, window_size.y - kRibbonNavigatorHeight});
    ribbon_textures.back().update(
        ribbon_frame->get()->data[0],
        sf::Vector2u{static_cast<unsigned int>(ribbon_frame->get()->width),
                     static_cast<unsigned int>(ribbon_frame->get()->height)},
        sf::Vector2u{0, 0});

    ribbon_sprites.emplace_back(ribbon_textures.back());

    const auto texture_size = ribbon_textures.back().getSize();

    ribbon_sprites.back().setScale(
        {(static_cast<float>(window_size.x) / segments_count) / texture_size.x,
         static_cast<float>(kRibbonNavigatorHeight) / texture_size.y});

    const auto index = std::distance(ribbon_frames.begin(), ribbon_frame);

    const float sprite_width =
        static_cast<float>(window_size.x) / segments_count;
    const float sprite_pos_x = index * sprite_width;
    const float sprite_pos_y =
        static_cast<float>(window_size.y - kRibbonNavigatorHeight);

    ribbon_sprites.back().setPosition({sprite_pos_x, sprite_pos_y});
  }

  auto rgb_frame = MakeAvFrameUnique(nullptr);

  // run the program as long as the window is open
  while (window.isOpen()) {
    // check all the window's events that were triggered since the last
    // iteration of the loop
    while (const auto event = window.pollEvent()) {
      // "close requested" event: we close the window
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

      if (const auto *resized = event->getIf<sf::Event::Resized>()) {
        // update the view to the new size of the window
        sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resized->size));
        window.setView(sf::View(visibleArea));
      }
    }

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
      sf::Vector2i localPosition = sf::Mouse::getPosition(window);
      if (localPosition.y >= video_stream->GetFrameSize().value().second) {
        auto x = std::clamp(
            localPosition.x, 0,
            static_cast<int>(video_stream->GetFrameSize().value().first));
        double time = static_cast<double>(x) /
                      video_stream->GetFrameSize().value().first *
                      video_stream->GetVideoStreamDuration().value();
        std::cout << std::format("Shifting to: {:.4f}", time) << std::endl;
      }
    }

    auto maybe_rgba_frame = video_stream->GetRgbaFrame(std::move(rgb_frame));
    if (maybe_rgba_frame.has_value()) {
      rgb_frame = *std::move(maybe_rgba_frame);
    } else {
      rgb_frame.reset();
    }

    // clear the window with black color
    window.clear(sf::Color::Black);

    // draw everything here...
    if (rgb_frame) {
      texture.update(rgb_frame->data[0],
                     sf::Vector2u{static_cast<unsigned int>(rgb_frame->width),
                                  static_cast<unsigned int>(rgb_frame->height)},
                     sf::Vector2u{0, 0});
    }

    window.draw(sprite);

    for (const auto &ribbon_sprite : ribbon_sprites) {
      window.draw(ribbon_sprite);
    }

    // end the current frame
    window.display();
  }
}