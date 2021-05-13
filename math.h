#pragma once

#include <limits>
#include <vector>
#include <string_view>
#include <cstring>

// Constants

static constexpr float infinity = std::numeric_limits<float>::infinity();
static constexpr float machine_epsilon = std::numeric_limits<float>::epsilon() * 0.5;
static constexpr float max_float = std::numeric_limits<float>::max();
static constexpr float pi{3.141592653f};

// Utility Functions

float random_float();
float random_float(float min, float max);
float standard_normal_random_float();

float fast_inverse_sqrt(float x);

inline float degrees_to_radians(float degrees) { return degrees * pi / 180.0f; }

inline float clamp(float value, float min, float max)
{
  if (value < min) return min;
  if (value > max) return max;
  return value;
}

namespace base64
{
  std::vector<unsigned char> decode(const std::string_view& encoded_string);
}

inline constexpr float gamma_bound(int n)
{
    return (n * machine_epsilon) / (1 - n * machine_epsilon);
}

inline uint32_t float_to_bits(float f)
{
  uint32_t bits;
  memcpy(&bits, &f, sizeof(float));
  return bits;
}

inline float bits_to_float(uint32_t bits)
{
  float f;
  memcpy(&f, &bits, sizeof(uint32_t));
  return f;
}

inline float next_float_up(float f)
{
  if (std::isinf(f) && f > 0.0f)
    return f;
  if (f == -0.0f)
    f = 0.0f;

  uint32_t bits = float_to_bits(f);
  if (f >= 0)
    ++bits;
  else
    --bits;

  return bits_to_float(bits);
}

inline float next_float_down(float f)
{
  if (std::isinf(f) && f < 0.0f)
    return f;
  if (f == 0.0f)
    f = -0.0f;

  uint32_t bits = float_to_bits(f);
  if (f > 0)
      --bits;
  else
      ++bits;

  return bits_to_float(bits);
}