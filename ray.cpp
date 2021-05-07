#include "ray.h"

ray::ray(const point& orig, const normed_vec3& dir) : origin{orig}, direction{dir} {}

point ray::at(float t) const
{
  point p = origin + t * direction.to_vec3();
  return p;
}