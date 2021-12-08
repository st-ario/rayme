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

void image::linear_to_srgb()
{
  constexpr float g{1.0f/2.2f};

  for(float& x : image_buffer)
    x = std::pow(x,g);
}

void image::hdr_to_ldr()
{
  // adapted from:
  //================================================================================================
  //
  //  Baking Lab
  //  by MJP and David Neubelt
  //  http://mynameismjp.wordpress.com/
  //
  //  All code licensed under the MIT license
  //
  //================================================================================================

  // Code originally written by Stephen Hill (@self_shadow), who deserves all
  // credit for coming up with this fit and implementing it.
  /*

  MIT License

  Copyright (c) 2016 MJP

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  */

  // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
  static constexpr glm::mat3
  ACESInputMat
  {
    {0.59719, 0.07600, 0.02840},
    {0.35458, 0.90834, 0.13383},
    {0.04823, 0.01566, 0.83777}
  };

  // ODT_SAT => XYZ => D60_2_D65 => sRGB
  static const glm::mat3
  ACESOutputMat
  {
    { 1.60475, -0.10208, -0.00327},
    {-0.53108,  1.10813, -0.07276},
    {-0.07367, -0.00605,  1.07602}
  };

  static constexpr auto RRTAndODTFit = [](const color& c) -> vec3
  {
      vec3 a = c * (c + 0.0245786f) - 0.000090537f;
      vec3 b = c * (0.983729f * c + 0.4329510f) + 0.238081f;
      return a / b;
  };

  for (size_t i = 0; i < image_buffer.size(); i+=3)
  {
    color c{image_buffer[i],image_buffer[i+1],image_buffer[i+2]};
    c = ACESInputMat * c;

    // Apply RRT and ODT
    c = RRTAndODTFit(c);

    c = ACESOutputMat * c;

    image_buffer[i]   = clamp(c.r, 0.0f, 1.0f);
    image_buffer[i+1] = clamp(c.g, 0.0f, 1.0f);
    image_buffer[i+2] = clamp(c.b, 0.0f, 1.0f);
  }
}

uint16_t image::get_height() const { return height; }
uint16_t image::get_width()  const { return width;  }