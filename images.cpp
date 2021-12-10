#include "images.h"

#include <cstdint>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "extern/stb/stb_image_write.h"

// select tonemapping curve (only one)
#define TONEMAP_REINHARD_1 1
//#define TONEMAP_HABLE_UNCHARTED2 1

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
  // https://entropymine.com/imageworsener/srgbformula/
  constexpr float conversion_exponent{1.0f/2.4f};
  for(float& x : image_buffer)
  {
    if (x < 0.0031308f)
      x *= 12.92f;
    else
      x = 1.055f * std::pow(x,conversion_exponent) - 0.055f;
  }
}

uint16_t image::luminance_to_bin(float luminance)
{
  // avoid log(0)
  if (luminance < 0.005f)
    return 0.0f;

  // calculate the log luminance and remap it linearly to [0.0, 1.0]
  float log_lum{clamp((std::log(luminance) - min_log_lum) * inverse_log_lum_range, 0.0f, 1.0f)};

  // map [0, 1] to [1, 255]
  // the zeroth bin is handled by the edge case check above.
  return static_cast<uint16_t>(log_lum * 254.0f + 1.0f);
}

float image::bin_to_log_luminance(uint16_t bin)
{
  return ((bin - 1.0f) * log_lum_range / 254.0f) + min_log_lum;
}

float image::exposure()
{
  std::array<uint32_t,256> histogram;
  histogram.fill(0u);

  for (size_t i = 0; i < image_buffer.size(); i+=3)
  {
    color c{image_buffer[i],image_buffer[i+1],image_buffer[i+2]};
    // RGB to luminance
    float lum{dot(c,rgb_to_luma)};

    ++histogram[luminance_to_bin(lum)];
  }

  // dark percentage of pixels to ignore, value in [0.0,1.0]
  constexpr float low_threshold{0.05f};
  // bright percentage of pixels to ignore, value in [0.0,1.0]
  constexpr float high_threshold{0.05f};

  const size_t pxl_number{image_buffer.size() / 3u};

  // ignore the lowest low_threshold % of values
  size_t clamplow{static_cast<size_t>(pxl_number * low_threshold)};
  // ignore the top high_threshold % of values
  size_t clamphigh{pxl_number - static_cast<size_t>(pxl_number * high_threshold)};

  size_t total_pxl_sum{0u};
  size_t relevant_pxl_sum{0u};
  float log_avg{0.0f};
  float lum_min{-infinity};
  float lum_max{-infinity};

  for (size_t i = 0; i < histogram.size(); ++i)
  {
    total_pxl_sum += histogram[i];
    if (total_pxl_sum < clamplow)
      continue;
    if (total_pxl_sum > clamphigh)
    {
      lum_max = std::exp(bin_to_log_luminance(i-1));
      break;
    }
    if (lum_min == -infinity)
      lum_min = std::exp(bin_to_log_luminance(i));

    relevant_pxl_sum += histogram[i];
    log_avg += bin_to_log_luminance(i) * histogram[i];
  }

  if (relevant_pxl_sum == 0u)
    return 0.0f;

  log_avg /= static_cast<float>(relevant_pxl_sum);
  float avg = std::exp(log_avg);

  // Reinhard--Ward--Pattanaik--Debevec--Heidrich--Myszkowksi,
  // "High Dynamic Range Imaging"
  float exponent{(2.0f * std::log2(avg) - std::log2(lum_max) - std::log2(lum_min))
    / (std::log2(lum_max) - std::log2(lum_min))};

  float alpha{0.18f * std::pow(4.0f, exponent)};

  return alpha / avg;
}

void image::hdr_to_ldr(bool autoexposure)
{
  #ifdef TONEMAP_HABLE_UNCHARTED2
  // Uncharted 2 tonemapping curve, credits to John Hable
  // http://filmicworlds.com/blog/filmic-tonemapping-operators/
  constexpr float A = 0.15f;
  constexpr float B = 0.50f;
  constexpr float C = 0.10f;
  constexpr float D = 0.20f;
  constexpr float E = 0.02f;
  constexpr float F = 0.30f;
  constexpr float W = 11.2f;

  auto static constexpr map = [=](float x){ return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F; };
  #endif
  #ifdef TONEMAP_REINHARD_1
  auto static constexpr map = [](float x){ return x / (x + 1.0f); };
  #endif

  float scale;
  if (autoexposure)
    scale = exposure();
  else
    scale = 1.0f;

  for (size_t i = 0; i < image_buffer.size(); i+=3)
  {
    #ifdef TONEMAP_HABLE_UNCHARTED2
    if (autoexposure)
    {
      image_buffer[i]   *= scale;
      image_buffer[i+1] *= scale;
      image_buffer[i+2] *= scale;
    }
    image_buffer[i]   = map(image_buffer[i]  );
    image_buffer[i+1] = map(image_buffer[i+1]);
    image_buffer[i+2] = map(image_buffer[i+2]);
    #endif

    #ifdef TONEMAP_REINHARD_1
    color k{image_buffer[i],image_buffer[i+1],image_buffer[i+2]};

    float l_w{dot(k,rgb_to_luma)};
    float l_m{autoexposure ? scale * l_w : l_w};

    float correction{map(l_m) / l_w};

    image_buffer[i]   *= correction;
    image_buffer[i+1] *= correction;
    image_buffer[i+2] *= correction;
    #endif
  }
}

uint16_t image::get_height() const { return height; }
uint16_t image::get_width()  const { return width;  }