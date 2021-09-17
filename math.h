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
static constexpr float two_pi{2.0f*pi};

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
    bool operator==(const normed_vec3& v) const { return static_cast<vec3>(*this) == static_cast<vec3>(v) ;}

    static normed_vec3 absolute_x() { return normed_vec3{1.0f,0.0f,0.0f}; }
    static normed_vec3 absolute_y() { return normed_vec3{0.0f,1.0f,0.0f}; }
    static normed_vec3 absolute_z() { return normed_vec3{0.0f,0.0f,1.0f}; }

    // return unit vector corresponding to v
    friend normed_vec3 unit(const vec3& v);
    friend normed_vec3 permute(const normed_vec3& v, int x, int y, int z);

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
    // angle formed wrt the north pole
    friend normed_vec3 cos_weighted_random_upper_hemisphere_unit();
    // return a random unit vector in the upper hemisphere having the argument as north pole,
    // weighted by the cosine of the angle formed wrt the argument
    friend normed_vec3 cos_weighted_random_hemisphere_unit(const normed_vec3& normal);
};

inline bool near_zero(const vec3& v)
{
  const float epsilon = 1e-8;
  return (std::fabs(v.x) < epsilon) && (std::fabs(v.y) < epsilon) && (std::fabs(v.z) < epsilon);
}

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

// return uniformly distributed vector inside the unit disk
vec3 random_vec3_in_unit_disk();

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

inline vec3 random_vec3_in_unit_disk()
{
  float random_radius = random_float();
  return random_radius * random_unit();
}

inline normed_vec3 random_upper_hemisphere_unit()
{
  normed_vec3 res = random_unit();
  if (random_unit().y() < 0)
    return normed_vec3(res.x(), - res.y(), res.z());

  return res;
}

// return some rotation sending the north pole of the unit sphere to the argument
inline glm::mat3 rotate_given_north_pole(const normed_vec3& normal)
{
  glm::mat3 change_of_base;

  if (normal.x() < machine_two_epsilon && normal.z() < machine_two_epsilon)
  {
    if (normal.y() > machine_two_epsilon)
      return change_of_base;
    else
    {
      change_of_base = {-1.0f,  0.0f, 0.0f,
                         0.0f, -1.0f, 0.0f,
                         0.0f,  0.0f, 1.0f};
    }
  } else if (normal.y() < machine_two_epsilon && normal.z() < machine_two_epsilon) {
    if (normal.x() > machine_two_epsilon)
    {
      change_of_base = {  0.0f, 1.0f, 0.0f,
                         -1.0f, 0.0f, 0.0f,
                          0.0f, 0.0f, 1.0f};
    } else {
      change_of_base = { 0.0f, -1.0f, 0.0f,
                         1.0f,  0.0f, 0.0f,
                         0.0f,  0.0f, 1.0f};
    }
  } else {
    normed_vec3 rel_x{unit(cross(normal,normed_vec3::absolute_x()))};
    normed_vec3 rel_z{unit(cross(rel_x,normal))};
    change_of_base = {rel_x.to_vec3(), normal.to_vec3(), rel_z.to_vec3()};
  }

  return change_of_base;
}

inline normed_vec3 random_hemisphere_unit(const normed_vec3& normal)
{
  vec3 res = rotate_given_north_pole(normal) * static_cast<vec3>(random_upper_hemisphere_unit());

  return normed_vec3(res[0],res[1],res[2]);
}

inline normed_vec3 cos_weighted_random_upper_hemisphere_unit()
{
  // picks a uniformly random point in the 2d disk and projects it to the hemisphere

  // random distribution on the circle (same trick as the 3d case)
  float x{standard_normal_random_float()};
  float z{standard_normal_random_float()};
  float inverse_norm{fast_inverse_sqrt(x*x + z*z)};
  x *= inverse_norm;
  z *= inverse_norm;

  // random radius
  float r{random_float()};
  x *= r;
  z *= r;

  // projection
  float y{std::sqrt(1.0f - x*x - z*z)};

  return normed_vec3{x, y, z};
}

inline normed_vec3 cos_weighted_random_hemisphere_unit(const normed_vec3& normal)
{
  vec3 res = rotate_given_north_pole(normal) * static_cast<vec3>(cos_weighted_random_upper_hemisphere_unit());

  return normed_vec3(res[0],res[1],res[2]);
}