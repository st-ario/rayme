#pragma once

#include "my_vectors.h"

struct ray
{
  point origin;
  normed_vec3 direction;

  ray() = delete;
  ray(const point& origin, const normed_vec3& direction) : origin{origin}, direction{direction} {}

  point at(float t) const
  {
    return origin + t * direction.to_vec3();
  }
};