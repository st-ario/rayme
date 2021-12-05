#pragma once

#include "math.h"

class image
{
  private:
    const uint16_t width;
    const uint16_t height;

  public:
    std::vector<float> image_buffer; // rows first, rgb

    image() = delete;

    // create uninitialized image
    image(uint16_t pixel_width, uint16_t pixel_height);

    // create image from image buffer
    image(uint16_t pixel_width, uint16_t pixel_height, std::vector<float>&& buffer);

    uint16_t get_height() const;
    uint16_t get_width() const;

    void write_to_png(const std::string& filename);
};