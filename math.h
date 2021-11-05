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
#include "extern/glm/glm/mat3x3.hpp"

// Constants

static constexpr float infinity = std::numeric_limits<float>::infinity();
static constexpr float machine_epsilon = std::numeric_limits<float>::epsilon() * 0.5;
static constexpr float machine_two_epsilon = std::numeric_limits<float>::epsilon();
static constexpr float max_float = std::numeric_limits<float>::max();
static constexpr float pi{3.141592653f};
static constexpr float pihalf{pi/2.0f};
static constexpr float pi_over_four{pi/4.0f};
static constexpr float two_pi{2.0f*pi};
static constexpr float invpi{1.0f/pi};

// Utility Functions

size_t random_size_t(size_t min, size_t max);
float random_float(uint16_t x, uint16_t y, uint16_t z);
float random_float(float min, float max, uint16_t x, uint16_t y, uint16_t z);
std::array<float,2> random_float_pair(uint16_t x, uint16_t y, uint16_t z);
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
  if (f == 0.0f && std::signbit(f)) // if (f == -0.0f)
    f = 0.0f;

  /* alternative, to avoid denormalized results
  // for zero and for positive denormalized floats, return the smallest normalized float
  if (f == 0 || (!std::signbit(f) && f < std::numeric_limits<float>::min()))
    return std::numeric_limits<float>::min();
  // for negative denormalized floats, return 0
  if (std::signbit(f) && f > -std::numeric_limits<float>::min())
    return 0.0f;
  */

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
  if (f == 0.0f && !std::signbit(f)) // if (f == +0.0f)
    f = -1.0f * 0.0f;

  /* alternative, to avoid denormalized results
  // for zero and for negative denormalized floats, return the negative normalized float closest
  // to zero
  if (f == 0 || (std::signbit(f) && f > -std::numeric_limits<float>::min()))
    return -std::numeric_limits<float>::min();
  // for positive denormalized floats, return 0
  if (!std::signbit(f) && f < std::numeric_limits<float>::min())
    return 0.0f;
  */

  uint32_t bits = float_to_bits(f);
  if (f > 0)
      --bits;
  else
      ++bits;

  return bits_to_float(bits);
}

// vectors

using vec3 = glm::vec3;
typedef glm::vec3 color;
typedef vec3 point;

class normed_vec3 : private vec3
{
  private:
    // unsafe constructor, doesn't check the invariant
    normed_vec3(float x, float y, float z) { this->r = x; this->g = y; this->b = z; }

  public:
    vec3 to_vec3() const { return static_cast<vec3>(*this); }

    // coordinates can only be returned by value, to preserve the invariant
    float x() const { return vec3::x; }
    float y() const { return vec3::y; }
    float z() const { return vec3::z; }
    float operator[](int i) const { return vec3::operator[](i); }

    normed_vec3 operator-() const;
    bool operator==(const normed_vec3& v) const
    {
      return static_cast<vec3>(*this) == static_cast<vec3>(v) ;
    }

    static normed_vec3 absolute_x() { return normed_vec3{1.0f,0.0f,0.0f}; }
    static normed_vec3 absolute_y() { return normed_vec3{0.0f,1.0f,0.0f}; }
    static normed_vec3 absolute_z() { return normed_vec3{0.0f,0.0f,1.0f}; }

    // return unit vector corresponding to v
    friend normed_vec3 unit(const vec3& v);

    friend float dot(const normed_vec3& v, const normed_vec3& w);
    friend float dot(const vec3& v,        const normed_vec3& w);
    friend float dot(const normed_vec3& v, const vec3& w);

    friend vec3 operator*(float x, const normed_vec3& w);
    friend vec3 operator*(const normed_vec3& v, float x);

    friend vec3 cross(const normed_vec3& v, const normed_vec3& w);
    friend vec3 cross(const vec3& v,        const normed_vec3& w);
    friend vec3 cross(const normed_vec3& v, const vec3& w);

    // return a uniformly distributed random unit vector in the absolute upper hemisphere
    friend normed_vec3 random_upper_hemisphere_unit();
    // return a uniformly distributed random unit vector in the hemisphere having the argument as
    // north pole
    friend normed_vec3 random_hemisphere_unit(const normed_vec3& normal);
    // return uniformly distributed vector on the unit sphere
    friend normed_vec3 random_unit();
    // return a random unit vector in the absolute upper hemisphere weighted by the cosine of the
    // angle formed wrt the north pole;
    // the three arguments are used for the rng
    friend normed_vec3
      cos_weighted_random_upper_hemisphere_unit(uint16_t pixel_x, uint16_t pixel_y, uint16_t seed);
    // return a random unit vector in the upper hemisphere having the argument as north pole,
    // weighted by the cosine of the angle formed wrt the argument;
    // the three extra arguments are used for the rng
    friend normed_vec3 cos_weighted_random_hemisphere_unit( const normed_vec3& normal
                                                          , uint16_t pixel_x
                                                          , uint16_t pixel_y
                                                          , uint16_t seed);
};

inline float epsilon_clamp(float value)
{
  if (std::fabs(value) < machine_two_epsilon)
    return 0.0f;
  return value;
}

inline vec3 epsilon_clamp(const vec3& v)
{
  return vec3{epsilon_clamp(v.x),epsilon_clamp(v.y),epsilon_clamp(v.z)};
}

inline normed_vec3 unit(const vec3& v)
{
  vec3 n = glm::normalize(v);
  return normed_vec3(n[0],n[1],n[2]);
}

uint8_t max_dimension(const vec3& v);
float max_component(const vec3& v);

normed_vec3 refract(const normed_vec3& incident, const normed_vec3& normal,
                    float refractive_indices_ratio);

void gamma2_correct(color& c);
void gamma_correct(color& c, float gamma);

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

inline vec3 cross(const normed_vec3& v, const normed_vec3& w)
{
  vec3 res = cross(static_cast<vec3>(v), static_cast<vec3>(w));
  return res;
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

inline void gamma_correct(color& c, float gamma)
{
  float g{1.0f/gamma};
  c.r = std::pow(c.r, g);
  c.g = std::pow(c.g, g);
  c.b = std::pow(c.b, g);
}

inline uint8_t max_dimension(const vec3& v)
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

/*
inline normed_vec3 random_unit()
{
  // computed normalizing standard Gaussians for each coordinate
  // to get the uniform distribution on the surface
  float rx = standard_normal_random_float();
  float ry = standard_normal_random_float();
  float rz = standard_normal_random_float();
  float inverse_norm = fast_inverse_sqrt(rx*rx + ry*ry + rz*rz);
  return normed_vec3(rx*inverse_norm,ry*inverse_norm,rz*inverse_norm);
}

inline normed_vec3 random_upper_hemisphere_unit()
{
  normed_vec3 res = random_unit();
  if (random_unit().y() < 0)
    return normed_vec3(res.x(), - res.y(), res.z());

  return res;
}
*/

// return some rotation sending the north pole of the absolute unit sphere to the argument
inline glm::mat3 rotate_given_north_pole(const normed_vec3& normal)
{
  glm::mat3 change_of_base;

  if (std::abs(normal.x()) < machine_two_epsilon && std::abs(normal.z()) < machine_two_epsilon)
  {
    if (normal.y() > 0.0f)
      return change_of_base;
    else
    {
      change_of_base = {-1.0f, 0.0f, 0.0f,
                         0.0f,-1.0f, 0.0f,
                         0.0f, 0.0f, 1.0f};
    }
  } else if (std::abs(normal.y()) < machine_two_epsilon
          && std::abs(normal.z()) < machine_two_epsilon) {
    if (normal.x() > 0.0f)
    {
      change_of_base = { 0.0f,-1.0f, 0.0f,
                         1.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 1.0f};
    } else {
      change_of_base = { 0.0f, 1.0f, 0.0f,
                        -1.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 1.0f};
    }
  } else {
    normed_vec3 rel_x{unit(cross(normal,normed_vec3::absolute_x()))};
    normed_vec3 rel_z{unit(cross(rel_x,normal))};
    change_of_base = {rel_x.to_vec3(), normal.to_vec3(), rel_z.to_vec3()};
  }

  return change_of_base;
}

/*
inline normed_vec3 random_hemisphere_unit(const normed_vec3& normal)
{
  vec3 res = rotate_given_north_pole(normal) * static_cast<vec3>(random_upper_hemisphere_unit());

  return normed_vec3(res[0],res[1],res[2]);
}
*/

inline normed_vec3
cos_weighted_random_upper_hemisphere_unit(uint16_t pixel_x, uint16_t pixel_y, uint16_t seed)
{
  // picks a uniformly random point in the 2d disk and projects it to the hemisphere

  // concentric disk sampling

  // map [0,1]^2 into [-1,1]^2
  auto rnd_pair{random_float_pair(pixel_x,pixel_y,seed)};
  float x{2.0f * rnd_pair[0] - 1.0f};
  float z{2.0f * rnd_pair[1] - 1.0f};

  // edge case
  if(x == 0.0f && z == 0.0f)
    return normed_vec3{0.0f,1.0f,0.0f};

  float angle{0.0f};
  float radius{0.0f};

  // concentric mapping formula
  if (std::abs(x) > std::abs(z))
  {
    radius = x;
    angle = pi_over_four * (z / x);
  } else {
    radius = z;
    angle = pihalf - pi_over_four * (x / z);
  }
  x = radius * std::cos(angle);
  z = radius * std::sin(angle);

  // projection
  float y{std::sqrt(1.0f - clamp(x*x - z*z, 0.0f, 1.0f))}; // clamping to avoid NaNs due to rounding

  return normed_vec3{x, y, z};
}

inline normed_vec3
cos_weighted_random_hemisphere_unit( const normed_vec3& normal
                                   , uint16_t pixel_x
                                   , uint16_t pixel_y
                                   , uint16_t seed)
{
  vec3 res{rotate_given_north_pole(normal)
    * cos_weighted_random_upper_hemisphere_unit(pixel_x, pixel_y, seed).to_vec3()};

  return normed_vec3(res[0],res[1],res[2]);
}