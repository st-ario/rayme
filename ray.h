#pragma once

#include "my_vectors.h"

class ray
{
  public:
    ray();
    // directions are always unital, if the argument is not a unit vector it is scaled by the
    // constructor before storing it
    ray(const point& origin, const vec& direction);

    point origin() const;
    vec direction() const;

    point& origin();

    // directions are always unital, if the argument is not a unit vector it is scaled by the
    // method before changing it
    ray& set_direction(const vec& new_direction);

    point at(double t) const;

  private:
    point orig;
    vec dir;
};