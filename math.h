#pragma once

#include <cmath>
#include <limits>
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>

#include "extern/glm/glm/glm.hpp"

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

// vectors

using vec3 = glm::vec3;
typedef glm::vec<3,double> color;
typedef vec3 point;

class normed_vec3 : public vec3
{
  private:
    // unsafe constructor, doesn't check the invariant
    normed_vec3(float w0, float w1, float w2);

  public:
    // construct a normed vector from an ordinary one by normalizing it
    explicit normed_vec3(const vec3& w);

    // coordinates can only be returned as const, to preserve the invariant
    const float x = vec3::x;
    const float y = vec3::y;
    const float z = vec3::z;

    normed_vec3& operator-();
    normed_vec3 operator-() const;

    static normed_vec3 random_unit();

    friend normed_vec3 permute(const normed_vec3& v, int x, int y, int z);
};

inline bool near_zero(const vec3& v)
{
  const float epsilon = 1e-8;
  return (std::fabs(v.x) < epsilon) && (std::fabs(v.y) < epsilon) && (std::fabs(v.z) < epsilon);
}

vec3 random_vec3_in_unit_sphere();

// return unit vector corresponding to v
inline normed_vec3 unit(const vec3& v) { return normed_vec3(v); }

int max_dimension(const vec3& v);
float max_component(const vec3& v);

inline vec3 abs(const vec3& v);

normed_vec3 reflect(const normed_vec3& incident, const normed_vec3& normal);
normed_vec3 refract(const normed_vec3& incident, const normed_vec3& normal,
                    float refractive_indices_ratio);

void gamma2_correct(color& c);
void gamma_correct(color& c, double gamma);

inline normed_vec3::normed_vec3(const vec3& w)
{
  float squared_norm = w[0]*w[0] + w[1]*w[1] + w[2]*w[2];
  float inverse_norm = fast_inverse_sqrt(squared_norm);
  this->operator[](0) = w[0] * inverse_norm;
  this->operator[](1) = w[1] * inverse_norm;
  this->operator[](2) = w[2] * inverse_norm;
}

inline normed_vec3::normed_vec3(float w0, float w1, float w2)
{
  this->operator[](0) = w0;
  this->operator[](1) = w1;
  this->operator[](2) = w2;
}

inline normed_vec3& normed_vec3::operator-()
{
  this->operator[](0) = - this->operator[](0);
  this->operator[](1) = - this->operator[](1);
  this->operator[](2) = - this->operator[](2);

  return *this;
}
inline normed_vec3 normed_vec3::operator-() const
{
  return normed_vec3(- this->operator[](0),
                     - this->operator[](1),
                     - this->operator[](2));
}

inline void gamma2_correct(color& c)
{
  c.r = std::sqrt(c.r);
  c.g = std::sqrt(c.g);
  c.b = std::sqrt(c.b);
}

inline void gamma_correct(color& c, double gamma)
{
  c.r = std::pow(c.r, 1.0/gamma);
  c.g = std::pow(c.g, 1.0/gamma);
  c.b = std::pow(c.b, 1.0/gamma);
}

inline int max_dimension(const vec3& v)
{
  if (v.x > v.y)
  {
    if (v.x > v.z)
      return 0;

    // z >= x > y
    return 2;
  }

  // y >= x
  if (v.y > v.z)
    return 1;
  // z >= y >= x
  return 2;
}

inline float max_component(const vec3& v)
{
  if (v.x > v.y)
  {
    if (v.x > v.z)
      return v.x;

    // z >= x > y
    return v.z;
  }

  // y >= x
  if (v.y > v.z)
    return v.y;
  // z >= y >= x
  return v.z;
}

inline float min_component(const vec3& v)
{
  if (v.x < v.y)
  {
    if (v.x < v.z)
      return v.x;

    return v.z;
  }

  if (v.y < v.z)
    return v.y;

  return v.z;
}

inline vec3 abs(const vec3& v)
{
  return vec3(std::abs(v[0]), std::abs(v[1]), std::abs(v[2]));
}

inline vec3 permute(const vec3& v, int x, int y, int z)
{
  return vec3{v[x], v[y], v[z]};
}

inline normed_vec3 permute(const normed_vec3& v, int x, int y, int z)
{
  return normed_vec3{v[x], v[y], v[z]};
}