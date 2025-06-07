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

int main() {
  const std::filesystem::path kBunnyFile =
      "./resources/BigBuckBunny36010s5MB.mp4";

  // create the window
  sf::RenderWindow window(sf::VideoMode({800, 600}), "My window");

  // Create an empty texture
  sf::Texture texture(sf::Vector2u{640, 360});

  // Create a sprite that will display the texture
  sf::Sprite sprite(texture);

  FrameExtractorFromFile frame_extractor{kBunnyFile};

  AvFrameUniquePtr rgb_frame = MakeAvFrameUnique(nullptr);

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

    if (frame_extractor.HasMoreVideoFrames()) {
      rgb_frame = frame_extractor.GetRgbaFrame(std::move(rgb_frame));
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