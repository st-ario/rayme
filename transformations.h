#pragma once

#include "math.h"
#include "extern/glm/glm/vec4.hpp"
#include "extern/glm/glm/mat4x4.hpp"
#include "extern/glm/glm/gtc/type_ptr.hpp"

using vec4 = glm::vec4;
using mat4 = glm::mat4;

vec4& operator*=(vec4& v, const mat4& m);

// to be used only for affine transformations
class transformation : public mat4
{
  public:
    using mat4::mat4;
    using mat4::operator=;

    transformation(const mat4& m) { *this = mat4{m}; }

    // slight abuse of notation: apply affine transformation to vec3
    vec3 operator*(const vec3& v) const;

    friend transformation rotation_matrix(const vec4& unit_quaternion);
    friend transformation translation_matrix(const vec3& translation_vector);
    friend transformation scale_matrix(const vec3& scale_vector);
    friend transformation scale_matrix(float scale_factor);
};

vec3& operator*=(vec3& v, const transformation& m);

// definitions

inline vec4& operator*=(vec4& v, const mat4& m)
{
  v = m * v;
  return v;
}

inline vec3 transformation::operator*(const vec3& v) const
{
  vec3 res{v};
  res *= (*this);

  return res;
}

inline vec3& operator*=(vec3& v, const transformation& m)
{
  vec4 aux{v,1.0f};
  aux *= static_cast<mat4>(m);
  v = static_cast<vec3>(aux);
  return v;
}

inline transformation rotation_matrix(const vec4& q)
{
  return transformation{
    1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]),
    2.0f * (q[0] * q[1] + q[2] * q[3]),
    2.0f * (q[0] * q[2] - q[1] * q[3]),
    0.0f,
    2.0f * (q[0] * q[1] - q[2] * q[3]),
    1.0f - 2.0f * (q[0] * q[0] + q[2] * q[2]),
    2.0f * (q[1] * q[2] + q[0] * q[3]),
    0.0f,
    2.0f * (q[0] * q[2] + q[1] * q[3]),
    2.0f * (q[1] * q[2] - q[0] * q[3]),
    1.0f - 2.0f * (q[0] * q[0] + q[1] * q[1]),
    0.0f,
    0.0f, 0.0f, 0.0f,1.0f
  };
}

inline transformation translation_matrix(const vec3& translation_vector)
{
  return transformation{ 1.0f,0.0f,0.0f,0.0f
                       , 0.0f,1.0f,0.0f,0.0f
                       , 0.0f,0.0f,1.0f,0.0f
                       , translation_vector.x
                       , translation_vector.y
                       , translation_vector.z
                       , 1.0f};
}

inline transformation scale_matrix(const vec3& scale_vector)
{
  return transformation{ scale_vector.x, 0.0f,0.0f,0.0f
                       , 0.0f, scale_vector.y,0.0f,0.0f
                       , 0.0f,0.0f, scale_vector.z,0.0f
                       , 0.0f,0.0f,0.0f,1.0f};
}

inline transformation scale_matrix(float scale_factor)
{
  return transformation{ scale_factor, 0.0f,0.0f,0.0f
                       , 0.0f, scale_factor,0.0f,0.0f
                       , 0.0f,0.0f, scale_factor,0.0f
                       , 0.0f,0.0f,0.0f,1.0f};
}