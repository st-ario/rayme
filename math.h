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

#define GLM_FORCE_CTOR_INIT
#include "extern/glm/glm/vec3.hpp"
#include "extern/glm/glm/geometric.hpp"

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

class normed_vec3 : private vec3
{
  private:
    // unsafe constructor, doesn't check the invariant
    using vec3::vec3;

  public:
    vec3 to_vec3() const { return static_cast<vec3>(*this); }

    // coordinates can only be returned by value, to preserve the invariant
    float x() const { return vec3::x; }
    float y() const { return vec3::y; }
    float z() const { return vec3::z; }
    float operator[](int i) const { return vec3::operator[](i); }

    static normed_vec3 random_unit();

    normed_vec3 operator-() const;
    bool operator==(const normed_vec3& v) const { return static_cast<vec3>(*this) == static_cast<vec3>(v) ;}

    // return unit vector corresponding to v
    friend normed_vec3 unit(const vec3& v);
    friend normed_vec3 permute(const normed_vec3& v, int x, int y, int z);

    friend float dot(const normed_vec3& v, const normed_vec3& w);
    friend float dot(const vec3& v,        const normed_vec3& w);
    friend float dot(const normed_vec3& v, const vec3& w);

    friend vec3  operator*(float x, const normed_vec3& w);
    friend vec3  operator*(const normed_vec3& v, float x);

    friend normed_vec3 cross(const normed_vec3& v, const normed_vec3& w);
    friend vec3        cross(const vec3& v,        const normed_vec3& w);
    friend vec3        cross(const normed_vec3& v, const vec3& w);
};

inline bool near_zero(const vec3& v)
{
  const float epsilon = 1e-8;
  return (std::fabs(v.x) < epsilon) && (std::fabs(v.y) < epsilon) && (std::fabs(v.z) < epsilon);
}

vec3 random_vec3_in_unit_sphere();

inline normed_vec3 unit(const vec3& v)
{
  vec3 n = glm::normalize(v);
  return normed_vec3(n[0],n[1],n[2]);
}

int max_dimension(const vec3& v);
float max_component(const vec3& v);

inline vec3 abs(const vec3& v);

normed_vec3 reflect(const normed_vec3& incident, const normed_vec3& normal);
normed_vec3 refract(const normed_vec3& incident, const normed_vec3& normal,
                    float refractive_indices_ratio);

void gamma2_correct(color& c);
void gamma_correct(color& c, double gamma);

inline normed_vec3 normed_vec3::operator-() const
{
  vec3 res = - static_cast<vec3>(*this);
  return normed_vec3(res[0],res[1],res[2]);
}

inline float dot(const normed_vec3& v, const normed_vec3& w)
{
  float res = dot(static_cast<vec3>(v), static_cast<vec3>(w));
  return res;
}

inline float dot(const vec3& v, const normed_vec3& w)
{
  float res = dot(v, static_cast<vec3>(w));
  return res;
}

inline float dot(const normed_vec3& v, const vec3& w)
{
  return dot(w,v);
}

inline vec3 operator*(float x, const normed_vec3& w)
{
  vec3 res = x * static_cast<vec3>(w);
  return res;
}

inline vec3 operator*(const normed_vec3& v, float x)
{
  return (x * v);
}

inline normed_vec3 cross(const normed_vec3& v, const normed_vec3& w)
{
  vec3 c = cross(static_cast<vec3>(v), static_cast<vec3>(w));
  return normed_vec3(c[0], c[1], c[2]);
}

inline vec3 cross(const vec3& v, const normed_vec3& w)
{
  vec3 res = cross(v, static_cast<vec3>(w));
  return res;
}

inline vec3 cross(const normed_vec3& v, const vec3& w)
{
  vec3 res = cross(static_cast<vec3>(v), w);
  return res;
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
  return vec3(std::fabs(v[0]), std::fabs(v[1]), std::fabs(v[2]));
}

inline vec3 permute(const vec3& v, int x, int y, int z)
{
  return vec3{v[x], v[y], v[z]};
}

inline normed_vec3 permute(const normed_vec3& v, int x, int y, int z)
{
  return normed_vec3{v[x], v[y], v[z]};
}