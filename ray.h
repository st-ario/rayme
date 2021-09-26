#pragma once

#include "math.h"

struct ray
{
  const point origin;
  const normed_vec3 direction;
  // vector storing 1/direction, used multiple times in hit checks
  // containes an infinity of the correct sign if the direction coordinate is 0
  const vec3 invD;
  // utility vector for the ray-triangle intersection function
  const glm::vec<3,uint8_t> perm;
  // vector storing coefficients for the ray-triangle intersection function
  const vec3 shear_coefficients;

  ray() = delete;
  ray(const point& origin, const normed_vec3& direction)
    : origin{origin}
    , direction{direction}
    , invD{ [&]{
        vec3 res;
        for (int i = 0; i < 3; ++i)
          res[i] = (direction[i] != 0.0f) ? (1.0f / direction[i]) : (direction[i] * infinity);
        return res;
      }() }
    , perm{ [&](){
        uint8_t kz{max_dimension(abs(direction.to_vec3()))};
        uint8_t kx{uint8_t(kz + 1)};
        if (kx == 3) kx = 0;
        uint8_t ky{uint8_t(kx + 1)};
        if (ky == 3) ky = 0;
        return glm::vec<3,uint8_t>{kx,ky,kz};
      }()}
    , shear_coefficients{ [&](){
        vec3 d = permute(direction.to_vec3(), perm.x, perm.y, perm.z);
        float Sx = - d.x  / d.z;
        float Sy = - d.y  / d.z;
        float Sz =   1.0f / d.z;
        return vec3{Sx,Sy,Sz};
      }() }
    {}

  point at(float t) const
  {
    return origin + t * direction;
  }
};

inline point offset_ray_origin(const point& p, const vec3& p_error, const normed_vec3& normal, const vec3& w)
{
    float d = dot(abs(normal.to_vec3()), p_error);
    vec3 offset = d * normal;

    offset = (dot(w, normal) < 0) ? -offset : offset;

    point offset_point = p + offset;

    // round offset point away from p
    for (int i = 0; i < 3; ++i)
    {
      if (offset[i] > 0)
        offset_point[i] = next_float_up(offset_point[i]);
      else if (offset.x < 0)
        offset_point[i] = next_float_down(offset_point[i]);
    }

    return offset_point;
}