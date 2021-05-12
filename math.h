#pragma once

#include <limits>
#include <vector>
#include <string_view>

// Constants

static constexpr float infinity = std::numeric_limits<float>::infinity();
static constexpr float pi{3.141592653f};
static constexpr float machine_epsilon = std::numeric_limits<float>::epsilon();

// Utility Functions

float degrees_to_radians(float degrees);

float random_float();
float random_float(float min, float max);
float standard_normal_random_float();

float clamp(float value, float min, float max);

float fast_inverse_sqrt(float x);

namespace base64
{
  std::vector<unsigned char> decode(const std::string_view& encoded_string);
}

inline constexpr float gamma_bound(int n)
{
    return (n * machine_epsilon) / (1 - n * machine_epsilon);
}