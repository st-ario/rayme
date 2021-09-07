#pragma once

#include "math.h"

struct ray
{
  point origin;
  normed_vec3 direction;

  ray() = delete;
  ray(const point& origin, const normed_vec3& direction) : origin{origin}, direction{direction} {}

  static point offset_ray_origin(const point &p, const vec3 &perror,
                                 const normed_vec3 &n, const vec3 &w);

  point at(float t) const
  {
    return origin + t * static_cast<vec3>(direction);
  }
};

inline point ray::offset_ray_origin(const point &p, const vec3 &pError,
                                    const normed_vec3 &n, const vec3 &w)
{
    float d = glm::dot(abs(static_cast<vec3>(n)), pError);
    vec3 offset = d * static_cast<vec3>(n);

    if (glm::dot(w, n) < 0)
      offset = - offset;

    point po = p + offset;

    // Round offset point po away from p
    if (offset.x > 0)
      po.x = next_float_up(po.x);
    else if (offset.x < 0)
      po.x = next_float_down(po.x);
    if (offset.x > 0)
      po.y = next_float_up(po.x);
    else if (offset.x < 0)
      po.y = next_float_down(po.x);
    if (offset.x > 0)
      po.z = next_float_up(po.x);
    else if (offset.x < 0)
      po.z = next_float_down(po.x);

    return po;
}