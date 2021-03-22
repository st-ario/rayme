#include "ray.h"

ray::ray() : orig{0,0,0}, dir{0,0,0} {}
// directions are always unital, if the argument is not a unit vector it is scaled by the
// constructor before storing it
ray::ray(const point& origin, const vec& direction) : orig{origin}
{
  double nm = direction.norm();
  if ((nm ==0) || (nm == 1))
  {
    dir = direction;
  } else {
    dir = unit(direction);
  }
}

point ray::origin()   const { return orig; }
vec ray::direction() const { return dir;  }

point& ray::origin()   { return orig; }

// directions are always unital, if the argument is not a unit vector it is scaled by the
// method before changing it
ray& ray::set_direction(const vec& new_direction)
{
  double nm = new_direction.norm();
  if ((nm ==0) || (nm == 1))
  {
    dir = new_direction;
  } else {
    dir = unit(new_direction);
  }

  return *this;
}

point ray::at(double t) const
{
  point p = orig + t * dir;
  return p;
}