#include "images.h"

#include <cstdint>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "extern/stb/stb_image_write.h"

image::image(uint16_t pixel_width, uint16_t pixel_height)
: width{pixel_width}, height{pixel_height}, image_buffer(width * height * 3u) {}

image::image(uint16_t pixel_width, uint16_t pixel_height, std::vector<float>&& buffer)
: width{pixel_width}, height{pixel_height}, image_buffer{std::move(buffer)} {}

void image::write_to_png(const std::string& file_name)
{
  std::vector<uint8_t> pixels(image_buffer.size());

  for (size_t i = 0; i < image_buffer.size(); ++i)
    pixels[i] = static_cast<uint8_t>(255.0 * clamp(image_buffer[i], 0.0f, 1.0f));

  stbi_write_png((file_name + ".png").c_str(), width, height, 3, pixels.data(), width * 3);
}

uint16_t image::get_height() const { return height; }
uint16_t image::get_width()  const { return width;  }