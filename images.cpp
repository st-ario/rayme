#include "images.h"

#include <cstdint>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "extern/stb/stb_image_write.h"

image::image(uint16_t pixel_width, uint16_t pixel_height) : width{pixel_width}, height{pixel_height}
{
  pixels.reserve(pixel_height);
  for (uint16_t r = 0; r < pixel_height; ++r)
  {
    std::vector<color> row;
    row.reserve(pixel_width);

    for (uint16_t c = 0; c < pixel_width; ++c)
    {
      row.emplace_back(color(0,0,0));
    }
    pixels.emplace_back(std::move(row));
  }
}

void image::write_to_png(const std::string& file_name)
{
  uint8_t color_data[width * height * 3]; //3 channels, RGB
  size_t index = 0;
  for (std::vector<color> row : pixels)
  {
    for (color c : row)
    {
      color_data[index++] = static_cast<uint8_t>(255.0 * clamp(c.r, 0, 1.0));
      color_data[index++] = static_cast<uint8_t>(255.0 * clamp(c.g, 0, 1.0));
      color_data[index++] = static_cast<uint8_t>(255.0 * clamp(c.b, 0, 1.0));
    }
  }

  stbi_write_png((file_name + ".png").c_str(), width, height, 3, color_data, width * 3);
}

uint16_t image::get_height() const { return height; }
uint16_t image::get_width()  const { return width;  }