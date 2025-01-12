#include <SFML/Graphics.hpp>
#include <filesystem>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

int main() {
  const std::filesystem::path kBunnyFile =
      "./resources/BigBuckBunny36010s5MB.mp4";

  AVFormatContext* format_ctx = nullptr;
  if (avformat_open_input(&format_ctx, kBunnyFile.c_str(), nullptr, nullptr) !=
      0) {
    std::cerr << "Could not open file: " << kBunnyFile << std::endl;
    return -1;
  }

  if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
    std::cerr << "Could not retreive stream info." << std::endl;
    avformat_close_input(&format_ctx);
    return -1;
  }

  int video_stream_idx = -1;
  for (size_t i = 0; i < format_ctx->nb_streams; ++i) {
    if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_idx = i;
      break;
    }
  }

  if (video_stream_idx == -1) {
    std::cerr << "No video stream found in the file" << std::endl;
    avformat_close_input(&format_ctx);
    return -1;
  }

  AVCodecParameters* codec_params =
      format_ctx->streams[video_stream_idx]->codecpar;
  const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
  if (!codec) {
    std::cerr << "Unsupported codec!" << std::endl;
    avformat_close_input(&format_ctx);
    return -1;
  }

  AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx) {
    std::cerr << "Could not allocate codec context." << std::endl;
    avformat_close_input(&format_ctx);
    return -1;
  }

  if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
    std::cerr << "Failed to copy codec parameters to codec context."
              << std::endl;
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    return -1;
  }

  if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
    std::cerr << "Could not open codec." << std::endl;
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    return -1;
  }

  AVFrame* frame = av_frame_alloc();
  AVPacket* packet = av_packet_alloc();
  AVFrame* rgb_frame = av_frame_alloc();
  if (!frame || !packet || !rgb_frame) {
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    return -1;
  }

  int width = codec_params->width;
  int height = codec_params->height;
  uint8_t* buffer = reinterpret_cast<uint8_t*>(
      av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1)));
  av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer,
                       AV_PIX_FMT_RGBA, width, height, 1);

  SwsContext* sws_ctx =
      sws_getContext(width, height, codec_ctx->pix_fmt, width, height,
                     AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
  if (!sws_ctx) {
    std::cerr << "Can not get sws context." << std::endl;
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    return -1;
  }

  // create the window
  sf::RenderWindow window(sf::VideoMode({800, 600}), "My window");

  // Create an empty texture
  sf::Texture texture(sf::Vector2u{640, 360});

  // Create a sprite that will display the texture
  sf::Sprite sprite(texture);

  // run the program as long as the window is open
  while (window.isOpen()) {
    // check all the window's events that were triggered since the last
    // iteration of the loop
    while (const auto event = window.pollEvent()) {
      // "close requested" event: we close the window
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

      if (const auto* resized = event->getIf<sf::Event::Resized>()) {
        // update the view to the new size of the window
        sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resized->size));
        window.setView(sf::View(visibleArea));
      }
    }

    if (av_read_frame(format_ctx, packet) >= 0) {
      if (packet->stream_index == video_stream_idx) {
        if (avcodec_send_packet(codec_ctx, packet) == 0) {
          while (avcodec_receive_frame(codec_ctx, frame) == 0) {
            std::cout << "Decoded frame: "
                      << "Width=" << frame->width
                      << ", Height=" << frame->height
                      << ", Format=" << frame->format << std::endl;
            // Convert frame to RGBA
            sws_scale(sws_ctx, frame->data, frame->linesize, 0, height,
                      rgb_frame->data, rgb_frame->linesize);
            texture.update(rgb_frame->data[0]);
          }
        }
      }

      av_packet_unref(packet);
    }

    // clear the window with black color
    window.clear(sf::Color::Black);

    // draw everything here...
    window.draw(sprite);

    // end the current frame
    window.display();
  }

  av_free(buffer);
  av_frame_free(&frame);
  av_packet_free(&packet);
  avcodec_free_context(&codec_ctx);
  avformat_close_input(&format_ctx);
}