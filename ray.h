#pragma once

#include "math.h"

struct ray
{
  const point origin;
  const normed_vec3 direction;
  // vector storing 1/direction, used multiple times in hit checks
  // containes an infinity of the correct sign if the direction coordinate is 0
  const vec3 invD;
  // utility vector for numeric robustness of ray-aabb intersection
  const vec3 invD_pad;
  // utility vector storing the signs of invD, for ray-aabb intersection
  const std::array<bool,3> sign;
  // utility vector storing vertices permutation, for ray-triangle intersection
  const glm::vec<3,uint8_t> perm;
  // vector storing coefficients for the ray-triangle intersection function
  const vec3 shear_coefficients;

  ray() = delete;
  ray(const point& origin, const normed_vec3& direction)
    : origin{origin}
    , direction{direction}
    , invD{1.0f/direction.x(),1.0f/direction.y(),1.0f/direction.z()}
    , invD_pad{add_ulp_magnitude(invD.x,2),add_ulp_magnitude(invD.y,2),add_ulp_magnitude(invD.z,2)}
    , sign{invD.x < 0, invD.y < 0, invD.z < 0}
    , perm{ [&](){
        // calculate dimension where ray direction is maximal
        uint8_t kz{max_dimension(glm::abs(direction.to_vec3()))};
        uint8_t kx{uint8_t(kz + 1)}; if (kx == 3) kx = 0;
        uint8_t ky{uint8_t(kx + 1)}; if (ky == 3) ky = 0;

        // swap kx and ky to preserve the orientation of the triangle vertices
        if (direction[kz] < 0.0f)
          std::swap(kx,ky);

        return glm::vec<3,uint8_t>{kx,ky,kz};
      }() }
    , shear_coefficients{ [&](){
        float Sz = 1.0f / direction[perm.z];
        float Sx = direction[perm.x] * Sz;
        float Sy = direction[perm.y] * Sz;
        return vec3{Sx,Sy,Sz};
      }() }
    {}

  point at(float t) const
  {
    return origin + t * direction;
  }
  private:
    static inline float add_ulp_magnitude(float f, int ulps)
    {
      if (!std::isfinite(f)) return f;
      unsigned bits = *reinterpret_cast<unsigned*>(&f);
      bits += ulps;
      return *reinterpret_cast<float*>(&bits);
    }
};

inline point offset_ray_origin( const point& p
                              , const vec3& p_error
                              , const normed_vec3& normal
                              , const normed_vec3& direction)
{
  float d{dot(glm::abs(normal.to_vec3()), p_error)};

  vec3 offset{d * normal};

  offset = (dot(direction, normal) < 0) ? -offset : offset;

  point offset_point{p + offset};

  // round offset point away from p
  for (int i = 0; i < 3; ++i)
  {
    if (!std::signbit(offset[i])) // account for positive zero
      offset_point[i] = next_float_up(offset_point[i]);
    else if (std::signbit(offset[i])) // account for negative zero
      offset_point[i] = next_float_down(offset_point[i]);
  }

  return offset_point;
}

inline ray bounce_ray( const point& p
                     , const vec3& p_error
                     , const normed_vec3& normal
                     , const normed_vec3& direction)
{
  return ray(offset_ray_origin(p,p_error,normal,direction),direction);
}