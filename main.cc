#include <SFML/Graphics.hpp>
#include <filesystem>
#include <iostream>

#include "FrameExtractorFromFile.h"

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
  FrameExtractorFromFile frame_extractor{kBunnyFile};

  if (auto maybe_size = frame_extractor.GetFrameSize();
      maybe_size.has_value()) {
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
  sf::Texture texture(sf::Vector2u{640, 360});

  // Create a sprite that will display the texture
  sf::Sprite sprite(texture);

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

    auto maybe_rgba_frame = frame_extractor.GetRgbaFrame(std::move(rgb_frame));
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

    // end the current frame
    window.display();
  }
}