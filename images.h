#pragma once
#include "my_vectors.h"
#include <vector>

class image
{
  private:
    const int height;
    const int width;

  public:
    std::vector<std::vector<color>> pixels;

  public:
    image() = delete;
    image(int pixel_width, int pixel_height);

    int get_height() const;
    int get_width() const;

    void write_to_ppm(std::string filename);
    void write_to_png(std::string filename);
};