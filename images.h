#pragma once

#include "math.h"

class image
{
  private:
    const uint16_t width;
    const uint16_t height;

  public:
    std::vector<std::vector<color>> pixels;

  public:
    image() = delete;

    // create black image
    image(uint16_t pixel_width, uint16_t pixel_height);

    // create image from image buffer
    image(uint16_t pixel_width, uint16_t pixel_height, float* buffer);

    uint16_t get_height() const;
    uint16_t get_width() const;

    void write_to_png(const std::string& filename);
};