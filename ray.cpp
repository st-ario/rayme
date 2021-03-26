#include "ray.h"

ray::ray(const point& orig, const normed_vec& dir) : origin{orig}, direction{dir} {}

point ray::at(double t) const
{
  point p = origin + t * direction.to_vec();
  return p;
}