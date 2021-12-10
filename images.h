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
    void hdr_to_ldr(bool autoexposure);
    void linear_to_srgb();

  private:
    static uint16_t luminance_to_bin(float luminance);
    static float bin_to_log_luminance(uint16_t bin);
    float background_intensity();
    float exposure();

    // RGB to luminance : https://stackoverflow.com/a/596243
    // https://en.wikipedia.org/wiki/Luma_(video)
    // ITU-R Recommendation BT.709 :
    static constexpr vec3 rgb_to_luma{0.2126f, 0.7152f, 0.0722f};
    // ITU-R Recommendation BT.601 :
    // static constexpr vec3 rgb_to_luma{0.299f, 0.587f, 0.114f};
    static constexpr float min_log_lum{-8.0f};
    static constexpr float max_log_lum{18.0f};
    static constexpr float log_lum_range{max_log_lum-min_log_lum};
    static constexpr float inverse_log_lum_range{1.0f/log_lum_range};
};