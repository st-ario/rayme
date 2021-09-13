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
    image(uint16_t pixel_width, uint16_t pixel_height);

    uint16_t get_height() const;
    uint16_t get_width() const;

    void write_to_ppm(std::string filename);
    void write_to_png(std::string filename);
};